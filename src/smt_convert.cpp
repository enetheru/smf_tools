#include <fstream>
#include <string>
#include <sstream>
#include <vector>

#include <OpenImageIO/imagebuf.h>

#include "elog/elog.h"
#include "optionparser/optionparser.h"

#include "option_args.h"
#include "smt.h"
#include "smf.h"
#include "util.h"
#include "tilemap.h"
#include "tilecache.h"
#include "tiledimage.h"

enum optionsIndex
{
    UNKNOWN,
    HELP, VERBOSE, QUIET,
    OUTPUT, FORCE,
    TILEMAP,
    FILTER,
    TILESIZE,
    IMAGESIZE,
    SMTOUT,
    IMGOUT,
};

const option::Descriptor usage[] = {
    { UNKNOWN, 0, "", "", Arg::None,
        "USAGE: smt_convert [options] <source1> [source2...sourceN]> \n"
        "  eg. 'smt_convert -o myfile.smt tilesource1.smt tilesource2.jpg'\n"
        "\nOPTIONS:"},

    { HELP, 0, "h", "help", Arg::None, "  -h,  \t--help"
        "\tPrint usage and exit." },

    { VERBOSE, 0, "v", "verbose", Arg::None, "  -v  \t--verbose"
        "\tPrint extra information." },

    { QUIET, 0, "q", "quiet", Arg::None, "  -q  \t--quiet"
        "\tSuppress print output." },

    { OUTPUT, 0, "o", "output", Arg::Required, "  -o  \t--output <filename>"
        "\tFilename to save as, default is output.smt" },

    { FORCE, 0, "f", "force", Arg::None, "  -f  \t--force"
        "\tOverwrite files with the same output name" },

    { FILTER, 0, "f", "filter", Arg::Required, "  -f  \t--filter=1,2-n"
        "\tFilter input tile sources to only these values, filter syntax is in"
        " the form 1,2,3,n and 1-5,n-n and can be mixed, 1-300,350,400-900" },

    { TILEMAP, 0, "t", "tilemap", Arg::Required, "  -t  \t--tilemap=[.csv,.smf]"
        "\tReconstruction tilemap." },

    { IMAGESIZE, 0, "", "imagesize", Arg::Required, "\t--imagesize=XxY"
        "\tScale the constructed image to this size before splitting." },

    { TILESIZE, 0, "s", "tilesize", Arg::Required, "  -s  \t--tilesize=XxY"
        "\tSplit the constructed image into tiles of this size." },

    { SMTOUT, 0, "", "smt", Arg::None, "\t--smt"
        "\tSave tiles to smt file" },

    { IMGOUT, 0, "", "img", Arg::None, "\t--img"
        "\tSave tiles as images" },
    { 0, 0, 0, 0, 0, 0 }
};

int
main( int argc, char **argv )
{
    // == Variables ==
    bool overwrite = false;
    // temporary
    OpenImageIO::ImageBuf *tempBuf;
    OpenImageIO::ImageSpec tempSpec;
    tempSpec.nchannels = 4;
    tempSpec.set_format( OpenImageIO::TypeDesc::UINT8 );
    SMF *tempSMF = NULL;
    SMT *tempSMT = NULL;
    std::stringstream name;

    // source
    TileCache src_tileCache;
    TileMap src_tileMap;
    TiledImage src_tiledImage;
    std::vector<uint32_t> src_filter;
    uint32_t src_tile_width, src_tile_height;
    //uint32_t src_map_width, src_map_height;
    //uint32_t src_img_width, src_img_height;

    // output
    std::string outFileName = "output.smt";
    TileMap out_tileMap;
    uint32_t out_tile_width, out_tile_height;
    //uint32_t out_map_width, out_map_height;
    uint32_t out_img_width, out_img_height;

    // relative intermediate size
    uint32_t rel_tile_width, rel_tile_height;

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

    if( options[ FORCE ] )overwrite = true;

    if( (options[ IMGOUT ] && options[ SMTOUT ])
        || (!options[ IMGOUT ] && !options[ SMTOUT ]) ){
        LOG( ERROR ) << "must have only one output format --img or --smt";
        fail = true;
    }

    if( options[ TILESIZE ] ){
        valxval( options[ TILESIZE ].arg, out_tile_width, out_tile_height );
        if( (out_tile_width % 4) || (out_tile_height % 4) ){
            LOG( ERROR ) << "tilesize must be a multiple of 4x4";
            fail = true;
        }
    }

    if( options[ IMAGESIZE ] ){
        valxval( options[ IMAGESIZE ].arg, out_img_width, out_img_height );
        if( (out_img_width % 4) || (out_img_height % 4) ){
            LOG( ERROR ) << "imagesize must be a multiple of 4x4";
            fail = true;
        }
    }

    if( fail || parse.error() ){
        LOG( ERROR ) << "Options parsing.";
        exit( 1 );
    }

    if( options[ SMTOUT ] ){
        if( options[ OUTPUT ] ) outFileName = options[ OUTPUT ].arg;
        tempSMT = SMT::create( outFileName , overwrite );
        if(! tempSMT ) LOG( FATAL ) << "cannot overwrite existing file";
    }

    if( options[ IMGOUT ] ){
        if( options[ OUTPUT ] ) outFileName = options[ OUTPUT ].arg;
    }

    // == TILE CACHE ==
    for( int i = 0; i < parse.nonOptionsCount(); ++i ){
        DLOG( INFO ) << "adding " << parse.nonOption( i ) << " to tilecache.";
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
        DLOG( INFO ) << "no filter specified, using all tiles";
        src_filter.resize( src_tileCache.getNTiles() );
        for( unsigned i = 0; i < src_filter.size(); ++i ) src_filter[ i ] = i;
    }

    // == TILEMAP ==
    // Source the tilemap, or generate it
    if( options[ TILEMAP ] ){
        // attempt to load from smf
        if( SMF::test( options[ TILEMAP ].arg ) ){
            DLOG( INFO ) << "tilemap derived from smt";
            tempSMF = SMF::open( options[ TILEMAP ].arg );
            TileMap *temp_tileMap = tempSMF->getMap();
            src_tileMap = *temp_tileMap;
            delete temp_tileMap;
            delete tempSMF;
        }
        // attempt to load from csv
        else {
            src_tileMap.fromCSV( options[ TILEMAP ].arg );
            if( src_tileMap.width != 0 && src_tileMap.height != 0 ){\
                DLOG( INFO ) << "tilemap derived from csv";
            }
        }
        // check for failure
        if( src_tileMap.width == 0 || src_tileMap.height == 0 ){
            LOG( ERROR ) << "unable to open: " << options[ TILEMAP ].arg;
            exit( 1 );
        }
    }
    else {
        DLOG( INFO ) << "tilemap generated ";
        uint32_t squareSize = std::ceil( std::sqrt( src_filter.size() ) );
        src_tileMap.setSize( squareSize, squareSize );
        src_tileMap.consecutive();
    }

    // == Build TiledImage ==
    src_tiledImage.setTileMap( src_tileMap );
    src_tiledImage.tileCache = src_tileCache;
    src_tiledImage.setTileSize( src_tile_width, src_tile_height );
    LOG( INFO ) << "\n    Source Tiled Image:"
        << "\n\tFull size: " << src_tiledImage.getWidth() << "x" << src_tiledImage.getHeight()
        << "\n\tTile size: " << src_tiledImage.tileWidth << "x" << src_tiledImage.tileHeight
        << "\n\tTileMap size: " << src_tiledImage.tileMap.width << "x" << src_tiledImage.tileMap.height;

    // == IMAGESIZE ==
    if(! options[ IMAGESIZE ] ){
        out_img_width = src_tiledImage.getWidth();
        out_img_height = src_tiledImage.getHeight();
    }

    // == TILESIZE ==
    if(! options[ TILESIZE ] ){
        out_tile_width = src_tile_width;
        out_tile_height = src_tile_height;
        if( options[ IMGOUT ] ){
            out_tile_width = out_img_width;
            out_tile_height = out_img_height;
        }
    }

    if( out_img_width % out_tile_width || out_img_height % out_tile_height ){
        LOG( ERROR ) << "image size must be a multiple of image size"
            << "\n\timage size: " << out_img_width << "x" << out_img_height
            << "\n\ttile size: " << out_tile_width << "x" << out_tile_height;
        exit( 1 );
    }

    // == Output Tile Map ==
    out_tileMap.setSize( out_img_width / out_tile_width,
                         out_img_height / out_tile_height);

    LOG( INFO ) << "\n    Output Sizes: "
        << "\n\tFull Size: " << out_img_width << "x" << out_img_height
        << "\n\tTile Size: " << out_tile_width << "x" << out_tile_height
        << "\n\ttileMap Size: " << out_tileMap.width << "x" << out_tileMap.height;

    // work out the relative tile size
    float xratio = (float)src_tiledImage.getWidth() / (float)out_img_width;
    float yratio = (float)src_tiledImage.getHeight() / (float)out_img_height;
    DLOG( INFO ) << "Scale Ratio: " << xratio << "x" << yratio;

    rel_tile_width = out_tile_width * xratio;
    rel_tile_height = out_tile_height * yratio;
    DLOG( INFO ) << "Pre-scaled tile: " << rel_tile_width << "x" << rel_tile_height;

    tempSpec.width = out_tile_width;
    tempSpec.height = out_tile_height;

    if( options[ SMTOUT ] ){

        tempSMT->setTileSize( out_tile_width );
    }

    // == OUTPUT THE IMAGES ==
    for( uint32_t y = 0; y < out_tileMap.height; ++y ) {
        for( uint32_t x = 0; x < out_tileMap.width; ++x ){
            out_tileMap(x,y) = y * out_tileMap.height + x;
            DLOG( INFO ) << "Processing split (" << x << ", " << y << ")";

            name << "output_" << x << "_" << y << ".jpg";

            tempBuf = src_tiledImage.getRegion(
                x * rel_tile_width, y * rel_tile_height,
                x * rel_tile_width + rel_tile_width , y * rel_tile_height + rel_tile_height );

            scale( tempBuf, tempSpec );

            //TODO optimisation
            if( options[ SMTOUT ] ) tempSMT->append( tempBuf );
            if( options[ IMGOUT ] ) tempBuf->write( name.str() );

            tempBuf->clear();
            delete tempBuf;
            name.str( std::string() );
        }
    }

    std::fstream out_csv("output.csv", std::ios::out );
    out_csv << out_tileMap.toCSV();
    out_csv.close();

    delete[] buffer;
    delete[] options;
    return 0;
}
