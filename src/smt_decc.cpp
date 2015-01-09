#include "smt.h"
#include "smf.h"
#include "util.h"
#include "tilemap.h"
#include "tilecache.h"
#include "tiledimage.h"

#include "elog/elog.h"
#include "optionparser/optionparser.h"

#include <OpenImageIO/imagebuf.h>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>

// Argument tests //
////////////////////
struct Arg: public option::Arg
{
    static void printError(const char* msg1, const option::Option& opt,
            const char* msg2)
    {
        fprintf(stderr, "%s", msg1);
        fwrite(opt.name, opt.namelen, 1, stderr);
        fprintf(stderr, "%s", msg2);
    }

    static option::ArgStatus Unknown(const option::Option& option, bool msg)
    {
        if (msg) printError("Unknown option '", option, "'\n");
        return option::ARG_ILLEGAL;
    }

    static option::ArgStatus Required(const option::Option& option, bool msg)
    {
        if (option.arg != 0)
            return option::ARG_OK;

        if (msg) printError("Option '", option, "' requires an argument\n");
        return option::ARG_ILLEGAL;
    }

    static option::ArgStatus Numeric(const option::Option& option, bool msg)
    {
        char* endptr = 0;
        if (option.arg != 0 && strtof(option.arg, &endptr)){};
        if (endptr != option.arg && *endptr == 0)
            return option::ARG_OK;

        if (msg) printError("Option '", option,
                "' requires a numeric argument\n");
        return option::ARG_ILLEGAL;
    }
};

enum optionsIndex
{
    UNKNOWN,
    HELP, VERBOSE, QUIET,
    OVERWRITE,
    TILEMAP,
    FILTER,
    TILESIZE,
    IMAGESIZE,
    SMT,
    IMG,
};

const option::Descriptor usage[] = {
    { UNKNOWN, 0, "", "", Arg::None,
        "USAGE: smt_info <source1> [source2...sourceN]> \n"
        "  eg. 'smt_info myfile.smt'\n"
        "\nGENERAL OPTIONS:"},
    { HELP, 0, "h", "help", Arg::None,
        "  -h,  \t--help  \tPrint usage and exit." },
    { VERBOSE, 0, "v", "verbose", Arg::None,
        "  -v  \t--verbose  \tPrint extra information." },
    { UNKNOWN, 0, "", "", Arg::None,
        "\nINPUT OPTIONS:" },
    { FILTER, 0, "f", "filter", Arg::Required,
        "  -f  \t--filter=1,2-n  \tonly pull selected tiles" },
    { TILEMAP, 0, "t", "tilemap", Arg::Required,
        "  -t  \t--tilemap=[.csv,.smf]  \treconstruction tilemap." },
    { UNKNOWN, 0, "", "", Arg::None,
        "\nOUTPUT OPTIONS:" },
    { TILESIZE, 0, "s", "tilesize", Arg::Required,
        "  -s  \t--tilesize=XxY  \tXxY" },
    { IMAGESIZE, 0, "", "imagesize", Arg::Required,
        "\t--imagesize=XxY  \tXxY" },
    { SMT, 0, "", "smt", Arg::None,
        "\t--smt  \t" },
    { IMG, 0, "", "img", Arg::None,
        "\t--img  \t" },
    { 0, 0, 0, 0, 0, 0 }
};

int
main( int argc, char **argv )
{
    // Option parsing
    // ==============
    bool fail = false;
    argc -= (argc > 0); argv += (argc > 0);
    option::Stats stats( usage, argc, argv );
    option::Option* options = new option::Option[ stats.options_max ];
    option::Option* buffer = new option::Option[ stats.buffer_max ];
    option::Parser parse( usage, argc, argv, options, buffer );

    if( options[ HELP ] || argc == 0 ) {
        int columns = getenv( "COLUMNS" ) ? atoi( getenv( "COLUMNS" ) ) : 80;
        option::printUsage( std::cout, usage, columns );
        exit( 1 );
    }

    // setup logging level
    LOG::SetDefaultLoggerLevel( LOG::WARN );
    if( options[ VERBOSE ] )
        LOG::SetDefaultLoggerLevel( LOG::INFO );
    if( options[ QUIET ] )
        LOG::SetDefaultLoggerLevel( LOG::CHECK );

    // unknown options
    for( option::Option* opt = options[ UNKNOWN ]; opt; opt = opt->next() ){
        LOG( WARN ) << "Unknown option: " << std::string( opt->name,opt->namelen );
        fail = true;
    }

    // non options
    if( parse.nonOptionsCount() == 0 ){
        LOG( ERROR ) << "Missing input tilesources";
        fail = true;
    }

    if( (options[ IMG ] && options[ SMT ]) 
        || (!options[ IMG ] && !options[ SMT ]) ){
        LOG( ERROR ) << "must have only one output format --img or --smt";
        fail = true;
    }

    if( fail || parse.error() ){
        LOG( ERROR ) << "Options parsing.";
        exit( 1 );
    }

    // == Variables ==
    // temporary
    OpenImageIO::ImageBuf *tempBuf;
    SMF *tempSMF = NULL;
    //SMT *tempSMT = NULL;
    std::stringstream name;

    // source
    TileCache src_tileCache;
    TileMap src_tileMap;
    std::vector<uint32_t> src_filter;
    uint32_t src_tile_width, src_tile_height;
    //uint32_t src_map_width, src_map_height;
    //uint32_t src_img_width, src_img_height;

    // output
    uint32_t out_tile_width, out_tile_height;
    uint32_t out_map_width, out_map_height;
    uint32_t out_img_width, out_img_height;


    // == TILE CACHE ==
    for( int i = 0; i < parse.nonOptionsCount(); ++i ){
        LOG( INFO ) << "adding " << parse.nonOption( i ) << " to tilecache.";
        src_tileCache.addSource( parse.nonOption( i ) );
    }
    LOG( INFO ) << src_tileCache.getNTiles() << " tiles in cache";

    // == SOURCE TILE SIZE ==
    tempBuf = src_tileCache( 0 );
    src_tile_width = tempBuf->spec().width;
    src_tile_height = tempBuf->spec().height;
    delete tempBuf;

    // == FILTER ==
    if( options[ FILTER ] ){
        src_filter = expandString( options[ FILTER ].arg );
        if( src_filter.size() == 0 ) {
            LOG( ERROR ) << "failed to interpret filter string";
            exit( 1 );
        }
    }
    else {
        src_filter.resize( src_tileCache.getNTiles() );
        for( unsigned i = 0; i < src_filter.size(); ++i ) src_filter[ i ] = i;
    }

    // == TILEMAP ==
    // Source the tilemap, or generate it
    if( options[ TILEMAP ] ){
        // attempt to load from smf
        if( SMT::test( options[ TILEMAP ].arg ) ){
            LOG( INFO ) << "tilemap derived from smt";
            tempSMF = SMF::open( options[ TILEMAP ].arg );
            src_tileMap = *(tempSMF->getMap());
            delete tempSMF;
            tempSMF = NULL;
        }
        // attempt to load from csv
        else {
            src_tileMap.fromCSV( options[ TILEMAP ].arg );
            if( src_tileMap.width != 0 && src_tileMap.height != 0 ){\
                LOG( INFO ) << "tilemap derived from csv";
            }
        }
        // check for failure
        if( src_tileMap.width == 0 || src_tileMap.height == 0 ){
            LOG( ERROR ) << "unable to open: " << options[ TILEMAP ].arg;
            exit( 1 );
        }
    }
    else {
        LOG( INFO ) << "tilemap generated ";
        uint32_t squareSize = std::ceil( std::sqrt( src_filter.size() ) );
        src_tileMap.setSize( squareSize, squareSize );
        src_tileMap.consecutive();
        LOG( INFO ) << "src_tileMap: " << src_tileMap.width << "x" << src_tileMap.height;
    }

    // == TILESIZE ==
    if( options[ TILESIZE ] ){
        valxval( options[ TILESIZE ].arg, out_tile_width, out_tile_height );
    }
    else {
        out_tile_width = src_tile_width;
        out_tile_height = src_tile_height;
    }

    // == IMAGESIZE ==
    if( options[ IMAGESIZE ] ){
        valxval( options[ IMAGESIZE ].arg, out_img_width, out_img_height );
    }
    else {
        out_img_width = out_tile_width * src_tileMap.width;
        out_img_height = out_tile_height * src_tileMap.height;
    }

    if(! options[ TILESIZE ] ){
        if( options[ IMG ] ){
            out_tile_width = out_img_width;
            out_tile_height = out_img_height;
        }
    }

    // // == EXPORT TILES ==
    // if(! options[ TILEMAP ] ) {
    //     for( auto i = src_filter.begin(); i != src_filter.end(); ++i ) {
    //         if( *i >= src_tileCache.getNTiles() ) {
    //             LOG( WARN ) << "tile: " << *i << " is out of range.";
    //             continue;
    //         }
    //         tempBuf = src_tileCache.getOriginal( *i );
    //         //FIXME scale based on tilesize?
    //         name << "tile_" << *i << ".jpg";
    //         tempBuf->write( name.str() );
    //         name.str( std::string() );// clear the string
    //     }
    //     exit( 0 );
    // }

    // == Build TiledImage ==
    TiledImage src_tiledImage;
    src_tiledImage.setTileMap( src_tileMap );
    src_tiledImage.tileCache = src_tileCache;
    src_tiledImage.setTileSize( src_tile_width, src_tile_height );
    
    LOG( INFO ) << "\n    Source Tiled Image:"
        << "\n\tFull size: " << src_tiledImage.getWidth() << "x" << src_tiledImage.getHeight()
        << "\n\tTile size: " << src_tiledImage.tileWidth << "x" << src_tiledImage.tileHeight
        << "\n\tTileMap size: " << src_tiledImage.tileMap.width << "x" << src_tiledImage.tileMap.height;

    out_map_width = out_img_width / out_tile_width;
    out_map_height = out_img_height / out_tile_height;
             
    LOG( INFO ) << "\n    Output "
        << "\n\tFull Size: " << out_img_width << "x" << out_img_height
        << "\n\tTile Size: " << out_tile_width << "x" << out_tile_height
        << "\n\ttileMap Size" << out_map_width << "x" << out_map_height;
        
    for( uint32_t i = 0; i < out_map_height; ++i ) {
        for( uint32_t j = 0; j < out_map_width; ++j ){
            LOG( INFO ) << "Processing split (" << j << ", " << i << ")";
            tempBuf = src_tiledImage.getRegion(
                j * out_tile_width, i * out_tile_height,
                j * out_tile_width + out_tile_width , i * out_tile_height + out_tile_height );
            name << "split_" << j << "_" << i << ".jpg";
            tempBuf->write( name.str() );
            name.str( std::string() );
        }
    }

//FIXME fill out
    /* What I want to be able to do
        [*] extract the tiles from the file at original size
        [*] extract specific tiles at original size
        [*] extract only the tiles listed in a tilemap
        [*] use tilemap or collate(square)
            [*] construct large image
            [ ] construct scaled image
            [ ] construct scaled image, scale and split
    */

    return 0;
}
