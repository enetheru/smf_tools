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
};

const option::Descriptor usage[] = {
    { UNKNOWN, 0, "", "", Arg::None,
        "USAGE: smt_info <smt file> \n"
        "  eg. 'smt_info myfile.smt'\n"
        "\nGENERAL OPTIONS:"},
    { HELP, 0, "h", "help", Arg::None,
        "  -h,  \t--help  \tPrint usage and exit." },
    { VERBOSE, 0, "v", "verbose", Arg::None,
        "  -v  \t--verbose  \tPrint extra information." },
    { TILEMAP, 0, "t", "tilemap", Arg::Required,
        "  -t  \t--tilemap=[.csv,.smf,GENERATE]  \treconstruction tilemap." },
    { FILTER, 0, "f", "filter", Arg::Required,
        "  -f  \t--filter=1,2-n  \tonly pull selected tiles" },
    { TILESIZE, 0, "s", "tilesize", Arg::Required,
        "  -s  \t--tilesize=XxY  \tXxY" },
    { IMAGESIZE, 0, "", "imagesize", Arg::Required,
        "\t--imagesize=XxY  \tXxY" },
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

    // == IMAGESIZE ==
    uint32_t ix = 0, iy = 0;
    if( options[ IMAGESIZE ] ){
        valxval( options[ IMAGESIZE ].arg, ix, iy );
    }

    if( fail || parse.error() ){
        LOG( ERROR ) << "Options parsing.";
        exit( 1 );
    }

    // non options will be considered tile sources
    TileCache tileCache;
    for( int i = 0; i < parse.nonOptionsCount(); ++i ){
        LOG( INFO ) << "adding " << parse.nonOption( i ) << " to tilecache.";
        tileCache.addSource( parse.nonOption( i ) );
    }
    LOG( INFO ) << tileCache.getNTiles() << " tiles in cache";

    // == TILESIZE ==
    uint32_t dest_tw = 32, dest_th = 32;
    if( options[ TILESIZE ] ){
        valxval( options[ TILESIZE ].arg, dest_tw, dest_th );
    }

    // Source Tile Size
    uint32_t src_tw = 32, src_th = 32;
    OpenImageIO::ImageBuf *tile;
    tile = tileCache( 0 );
    src_tw = tile->spec().width;
    src_th = tile->spec().height;

    // == FILTER ==
    // resolve filter list
    std::vector<unsigned int> filter;
    if( options[ FILTER ] ) {
        filter = expandString( options[ FILTER ].arg );
    }
    else {// otherwise set the filter list to all tiles.
        filter.resize( tileCache.getNTiles() );
        for( unsigned int i = 0; i < filter.size(); ++i ) filter[i] = i;
    }
    //FIXME filter based on tilemap?

    // == EXPORT TILES ==
    OpenImageIO::ImageBuf *buf = NULL;
    std::stringstream name;
    if(! options[ TILEMAP ] ) {
        for( auto i = filter.begin(); i != filter.end(); ++i ) {
            if( *i >= tileCache.getNTiles() ) {
                LOG( WARN ) << "tile: " << *i << " is out of range.";
                continue;
            }
            buf = tileCache.getOriginal( *i );
            //FIXME scale based on tilesize?
            name << "tile_" << *i << ".jpg";
            buf->write( name.str() );
            name.str( std::string() );// clear the string
        }
        exit( 0 );
    }

    // == TILEMAP ==
    // Source the tilemap, or generate it
    TileMap tileMap;
    if( options[ TILEMAP ] ){
        SMF *smf = NULL;
        // attempt to load from smf
        if(! strcmp( options[ TILEMAP ].arg, "GENERATE" ) ){
            int tc = std::ceil( std::sqrt( filter.size() ) );
            tileMap.setSize( tc, tc );
            tileMap.consecutive();
        }
        else if( (smf = SMF::open( options[ TILEMAP ].arg )) ){
            tileMap = *(smf->getMap());
            delete smf;
        }
        else if(! smf ){ // attempt to load from csv
            std::ifstream inFile( options[ TILEMAP ].arg );
            std::string csv;

            inFile.seekg(0, std::ios::end);
            csv.reserve(inFile.tellg());
            inFile.seekg(0, std::ios::beg);

            csv.assign((std::istreambuf_iterator<char>(inFile)),
                std::istreambuf_iterator<char>());
            tileMap.fromCSV( csv );
            inFile.close();
        }
    }

    // == Build TiledImage ==
    TiledImage tiledImage;
    tiledImage.setTileMap( tileMap );
    tiledImage.tileCache = tileCache;
    tiledImage.setTileSize( src_tw, src_th );

    LOG( INFO ) << "\n    Source Tiled Image:"
        << "\n\tsize: " << tiledImage.w << "x" << tiledImage.h
        << "\n\ttile size: " << tiledImage.tw << "x" << tiledImage.th
        << "\n\ttileMap size: " << tiledImage.tileMap.width << "x" << tiledImage.tileMap.height;

    uint32_t sxc, syc;
    LOG( INFO ) << "\n    Converting "
        << "\n\tFull Size: " << tiledImage.w << "x" << tiledImage.h
        << "\n\tTile Size: " << dest_tw << "x" << dest_th
        << "\n\t" << (sxc = tiledImage.w / dest_tw) << " horizontal segments"
        << "\n\t" << (syc = tiledImage.h / dest_th) << " vertical segments";
    for( uint32_t i = 0; i < syc; ++i ) {
        for( uint32_t j = 0; j < sxc; ++j ){
            LOG( INFO ) << "Processing split (" << j << ", " << i << ")";
            buf = tiledImage.getRegion(
                j * dest_tw, i * dest_th,
                j * dest_tw + dest_tw , i * dest_th + dest_th );
            name << "split_" << j << "_" << i << ".jpg";
            buf->write( name.str() );
            name.str( std::string() );
        }
    }

//FIXME fill out
    /* What I want to be able to do
        [*] extract the tiles from the file at original size
        [*] extract specific tiles at original size
        [ ] extract only the tiles listed in a tilemap
        [ ] use tilemap or collate(square)
            [ ] construct large image
            [ ] construct scaled image
            [ ] construct scaled image, scale and split
    */

    return 0;
}
