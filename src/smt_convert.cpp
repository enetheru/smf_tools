#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>
#include <unordered_map>

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>

using OpenImageIO::ImageBufAlgo::computePixelHashSHA1;
using OpenImageIO::TypeDesc;

#include <elog.h>

#include "option_args.h"
#include "smt.h"
#include "smf.h"
#include "util.h"
#include "tilecache.h"
#include "tiledimage.h"

enum optionsIndex
{
    UNKNOWN,
    HELP,
    QUIET,
    VERBOSE,
    PROGRESS,

    OUTPUT_PATH,
    OUTPUT_NAME,
    OUTPUT_OVERWRITE,

    IMAGESIZE,
    TILESIZE,
    FORMAT,
    TILEMAP,

    FILTER,
    OVERLAP,
    BORDER,
    DUPLI,
    SMTOUT,
    IMGOUT,
};
//FIXME what happened to specifying the span of input tiles?
//FIXME now using output path and output name
const option::Descriptor usage[] = {
    { UNKNOWN, 0, "", "", Arg::None,
        "USAGE: smt_convert [options] <source1> [source2...sourceN]> \n"
        "  eg. 'smt_convert -o myfile.smt tilesource1.smt tilesource2.jpg'\n"
        "\nOPTIONS:"},

    { HELP,             0, "h", "help",       Arg::None,
"  -h,  \t--help\t"
"Print usage and exit." },
    { QUIET,            0, "q", "quiet",      Arg::None,
"  -q  \t--quiet\t"
"Suppress print output." },
    { VERBOSE,          0, "v", "verbose",    Arg::None,
"  -v  \t--verbose\t"
"Print extra information." },
    { PROGRESS,         0, "p", "progress",   Arg::None,
"  -p  \t--progress\t"
"Display progress indicator" },

    { OUTPUT_PATH,      0, "o", "output",     Arg::Required,
"  -o  \t--output <dir>\t"
"Filename to save as, default is output.smt" },
    { OUTPUT_NAME,      0, "n", "name",       Arg::Required,
"  -n  \t--name <filename>\t"
"Filename to save as, default is output.smt" },
    { OUTPUT_OVERWRITE, 0, "O", "overwrite",  Arg::None,
"  -O  \t--overwrite\t"
"Overwrite files with the same output name" },

    { IMAGESIZE,        0, "i", "imagesize",  Arg::Required,
"  -i  \t--imagesize=XxY\t"
"Scale the constructed image to this size before splitting." },
    { TILESIZE,         0, "t", "tilesize",   Arg::Required,
"  -t  \t--tilesize=XxY\t"
"Split the constructed image into tiles of this size." },
    { FORMAT,           0, "f", "format",     Arg::Required,
"  -f  \t--format=[DXT1,RGBA8,USHORT]\t"
"default=DXT1, what format to put into the smt"},
    { TILEMAP,          0, "M", "tilemap",        Arg::Required,
"  -M  \t--tilemap=<csv|smf>\t"
"Reconstruction tilemap." },

    { FILTER,           0, "e", "filter",   Arg::Required,
"  -e  \t--filter=1,2-n\t"
"Filter input tile sources to only these values, filter syntax is in the form"
"1,2,3,n and 1-5,n-n and can be mixed, 1-300,350,400-900" },
    { OVERLAP,          0, "k", "overlap",   Arg::Numeric,
"  -k  \t--overlap=0\t"
"consider that pixel values overlap by this amount." },
    { BORDER,           0, "b", "border",   Arg::Numeric,
"  -b  \t--border=0\t"
"consider that each tile has a border of this width" },
    { DUPLI,            0, "d", "dupli",   Arg::Required,
"  -d  \t--dupli=[None,Exact,Perceptual]\t"
"default=Exact, whether to detect and omit duplcates." },

    { SMTOUT,           0, "", "smt", Arg::None,
      "\t--smt\t"              "Save tiles to smt file" },
    { IMGOUT,           0, "", "img", Arg::None,
      "\t--img\t"              "Save tiles as images" },

    { 0, 0, 0, 0, 0, 0 }
};

int
main( int argc, char **argv )
{
    // == Variables ==
    bool overwrite = false;
    // temporary
    SMF *tempSMF = nullptr;
    SMT *tempSMT = nullptr;
    std::stringstream name;

    // source
    TileCache src_tileCache;
    TileMap src_tileMap;
    TiledImage src_tiledImage;
    std::vector<uint32_t> src_filter;
    OpenImageIO::ImageSpec sSpec;
    uint32_t overlap = 0;

    // output
    std::string out_fileName = "output.smt";
    std::string out_fileDir = "./";
    TileMap out_tileMap;
    uint32_t out_format = 1;
    OpenImageIO::ImageSpec otSpec( 32, 32, 4, TypeDesc::UINT8 );
    uint32_t out_img_width = 0, out_img_height = 0;

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
        options[ HELP ] ? exit( 0 ) : exit( 1 );
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

    if( options[ OUTPUT_OVERWRITE ] )overwrite = true;

    if( (options[ IMGOUT ] && options[ SMTOUT ])
        || (!options[ IMGOUT ] && !options[ SMTOUT ]) ){
        LOG( ERROR ) << "must have only one output format --img or --smt";
        fail = true;
    }

    // * Output Format
    if(  options[ FORMAT ] ){
        if( strcmp( options[ FORMAT ].arg, "DXT1" ) == 0 ){
            out_format = 1;
            otSpec.nchannels = 4;
            otSpec.set_format( TypeDesc::UINT8 );
        }

        if( strcmp( options[ FORMAT ].arg, "RGBA8" ) == 0 ){
            out_format = GL_RGBA8;
            otSpec.nchannels = 4;
            otSpec.set_format( TypeDesc::UINT8 );
        }

        if( strcmp( options[ FORMAT ].arg, "USHORT" ) == 0 ){
            out_format = GL_UNSIGNED_SHORT;
            otSpec.nchannels = 1;
            otSpec.set_format( TypeDesc::UINT16 );
        }
    }

    // * Tile Size
    if( options[ TILESIZE ] ){
        std::tie( otSpec.width, otSpec.height )
                = valxval( options[ TILESIZE ].arg );
        if( (out_format == 1)
            && ((otSpec.width % 4) || (otSpec.height % 4))
          ){
            LOG( ERROR ) << "tilesize must be a multiple of 4x4";
            fail = true;
        }
    }
    else if(! options[ IMGOUT ] ){
        otSpec.width = 32;
        otSpec.height = 32;
    }

    //FIXME since this tool is build primarily for springrts, then if --smt is
    //specified and no --imagesize round the image size based on input to the
    //closest multiple of 1024 > 0
    // * Image Size
    if( options[ IMAGESIZE ] ){
        std::tie( out_img_width, out_img_height )
                = valxval( options[ IMAGESIZE ].arg );
        if( (out_format == 1)
            && ((out_img_width % 4) || (out_img_height % 4))
          ){
            LOG( ERROR ) << "imagesize must be a multiple of 4x4";
            fail = true;
        }
    }

    if( options[ OVERLAP ] ){
        overlap = atoi( options[ OVERLAP ].arg );
    }

    // * Duplicate Detection
    int dupli = 1;
    if( options[ DUPLI ] ){
        if( strcmp( options[ DUPLI ].arg, "None" ) == 0 ) dupli = 0;
        if( strcmp( options[ DUPLI ].arg, "Perceptual" ) == 0 ) dupli = 2;
    }

    // * Output File
    if( options[ SMTOUT ] ){
        if( options[ OUTPUT_NAME ] ) out_fileName = options[ OUTPUT_NAME ].arg;
        tempSMT = SMT::create( out_fileName , overwrite );
        if(! tempSMT ) LOG( FATAL ) << "cannot overwrite existing file";
        tempSMT->setType( out_format );
    }

    if( options[ IMGOUT ] ){
        if( options[ OUTPUT_NAME ] ) out_fileName = options[ OUTPUT_NAME ].arg;
    }

    if( fail || parse.error() ){
        LOG( ERROR ) << "Options parsing.";
        exit( 1 );
    }

    // == TILE CACHE ==
    for( int i = 0; i < parse.nonOptionsCount(); ++i ){
        DLOG( INFO ) << "adding " << parse.nonOption( i ) << " to tilecache.";
        src_tileCache.addSource( parse.nonOption( i ) );
    }
    CHECK( src_tileCache.nTiles ) << "no tiles in cache";
    LOG( INFO ) << src_tileCache.nTiles << " tiles in cache";

    // == SOURCE TILE SPEC ==
    {
        std::unique_ptr< OpenImageIO::ImageBuf >
                tempBuf(src_tileCache.getTile(0) );
        sSpec.width = tempBuf->spec().width;
        sSpec.height = tempBuf->spec().height;
        sSpec.nchannels = otSpec.nchannels;
        sSpec.set_format( otSpec.format );
        LOG( INFO ) << "Source Tile Size: " << sSpec.width << "x" << sSpec.height;
    }

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
        src_filter.resize( src_tileCache.nTiles );
        for( unsigned i = 0; i < src_filter.size(); ++i ) src_filter[ i ] = i;
    }

    // == Source TILEMAP ==
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
        //TODO allow user to specify generation technique, ie stride,
        // or aspect ratio.
        DLOG( INFO ) << "no tilemap specified, generated one instead";
        uint32_t squareSize = std::ceil( std::sqrt( src_filter.size() ) );
        src_tileMap.setSize( squareSize, squareSize );
        src_tileMap.consecutive();
    }

    // == Build source TiledImage ==
    src_tiledImage.setTileMap( src_tileMap );
    src_tiledImage.tileCache = src_tileCache;
    src_tiledImage.setTSpec( sSpec );
    src_tiledImage.setOverlap( overlap );
    LOG( INFO ) << "\n    Source Tiled Image:"
        << "\n\tFull size: " << src_tiledImage.getWidth() << "x" << src_tiledImage.getHeight()
        << "\n\tTile size: " << src_tiledImage.tSpec.width << "x" << src_tiledImage.tSpec.height
        << "\n\tTileMap size: " << src_tiledImage.tileMap.width << "x" << src_tiledImage.tileMap.height;

    // == IMAGESIZE ==
    if(! options[ IMAGESIZE ] ){
        out_img_width = src_tiledImage.getWidth();
        out_img_height = src_tiledImage.getHeight();
    }
    LOG( INFO ) << "ImageSize = " << out_img_width << "x" << out_img_height;

    // == TILESIZE ==
    // TODO if smt is specified then default to 32x32
    if(! options[ TILESIZE ] ){
        if( options[ IMGOUT ] ){
            otSpec.width = out_img_width;
            otSpec.height = out_img_height;
        }
        else {
            otSpec.width = 32;
            otSpec.height = 32;
        }
    }

/*    if( out_img_width % otSpec.width
            || out_img_height % otSpec.height ){
        LOG( ERROR ) << "image size must be a multiple of tile size"
            << "\n\timage size: " << out_img_width << "x" << out_img_height
            << "\n\ttile size: " << otSpec.width << "x" << otSpec.height;
        exit( 1 );
    }*/

    // == prepare output Tile Map ==
    out_tileMap.setSize( out_img_width / otSpec.width,
                         out_img_height / otSpec.height);

    LOG( INFO ) << "\n    Output Sizes: "
        << "\n\tFull Size: " << out_img_width << "x" << out_img_height
        << "\n\tTile Size: " << otSpec.width << "x" << otSpec.height
        << "\n\ttileMap Size: " << out_tileMap.width << "x" << out_tileMap.height;

    // work out the relative tile size
    float xratio = (float)src_tiledImage.getWidth() / (float)out_img_width;
    float yratio = (float)src_tiledImage.getHeight() / (float)out_img_height;
    DLOG( INFO ) << "Scale Ratio: " << xratio << "x" << yratio;

    rel_tile_width = otSpec.width * xratio;
    rel_tile_height = otSpec.height * yratio;
    DLOG( INFO ) << "Pre-scaled tile: " << rel_tile_width << "x" << rel_tile_height;

    if( options[ SMTOUT ] )tempSMT->setTileSize( otSpec.width );

    // tile hashtable for exact duplicate detection
    std::unordered_map<std::string, int> hash_map;
    int *item;
    hash_map.reserve(out_tileMap.width * out_tileMap.height);

    // == OUTPUT THE IMAGES ==
    int numTiles = 0;
    int numDupes = 0;
    OpenImageIO::ROI roi = OpenImageIO::ROI::All();
    std::unique_ptr< OpenImageIO::ImageBuf > outBuf;
    for( uint32_t y = 0; y < out_tileMap.height; ++y ) {
        for( uint32_t x = 0; x < out_tileMap.width; ++x ){
            DLOG( INFO ) << "Processing split (" << x << ", " << y << ")";

            roi.xbegin = x * rel_tile_width;
            roi.xend   = x * rel_tile_width + rel_tile_width;
            roi.ybegin = y * rel_tile_height;
            roi.yend   = y * rel_tile_height + rel_tile_height;
            outBuf = src_tiledImage.getRegion( roi );

            if( dupli == 1 ){
                item = &hash_map[ computePixelHashSHA1( *outBuf ) ];
                if( *item ){
                    out_tileMap( x, y ) = *item;
                    ++numDupes;
                    continue;
                }
                else {
                    *item = numTiles;
                }
            }

            // scale according to otSpec, which is conditionally defined by
            // --tilesize or --imagesize depending on whether one image is
            // being exported or whether to split up into chunks.
            outBuf = fix_scale( std::move( outBuf ), otSpec );

            if( options[ SMTOUT ] ) tempSMT->append( *outBuf );
            if( options[ IMGOUT ] ){
                name << "tile_" << std::setfill('0') << std::setw(6) << numTiles << ".tif";
                outBuf->write( name.str() );
                name.str( std::string() );
            }
            out_tileMap(x,y) = numTiles;
            ++numTiles;

            if( options[ PROGRESS ] ){
                progressBar( "[Progress]:",
                    out_tileMap.width * out_tileMap.height - numDupes,
                    numTiles );
            }
        }
    }
    LOG(INFO) << "actual:max = " << numTiles << ":" << out_tileMap.width * out_tileMap.height;
    LOG(INFO) << "number of dupes = " << numDupes;

    // if the tileMap only contains 1 value, then we are only outputting
    //     a single image, so skip tileMap csv export
    if( out_tileMap.size() > 1 ){
        std::fstream out_csv( out_fileName + ".csv", std::ios::out );
        out_csv << out_tileMap.toCSV();
        out_csv.close();
    }

    delete[] buffer;
    delete[] options;
    return 0;
}
