#include <fstream>
#include <vector>
#include <iostream>
#include <unordered_map>

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>

using OIIO::ImageBufAlgo::computePixelHashSHA1;
using OIIO::TypeDesc;

#include <spdlog/spdlog.h>

#include "option_args.h"
#include "smflib/smt.h"
#include "smflib/smf.h"
#include "smflib/util.h"
#include "smflib/tilecache.h"
#include "smflib/tiledimage.h"

enum optionsIndex
{
    UNKNOWN,
    HELP,
    QUIET,
    VERBOSE,
    PROGRESS,

    OUTPUT_PATH,
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
        {UNKNOWN,          0, "",      "",          Arg::None,
                                                             "USAGE: smt_convert [options] <source1> [source2...sourceN]> \n"
                                                             "  eg. 'smt_convert -o myfile.smt tilesource1.smt tilesource2.jpg'\n"
                                                             "\nOPTIONS:"},

        {HELP,             0, "h",     "help",      Arg::None,
                                                             "  -h,  \t--help\t"
                                                             "Print usage and exit."},
        {QUIET,            0, "q",     "quiet",     Arg::None,
                                                             "  -q  \t--quiet\t"
                                                             "Suppress print output."},
        {VERBOSE,          0, "v",     "verbose",   Arg::None,
                                                             "  -v  \t--verbose\t"
                                                             "Print extra information."},
        {PROGRESS,         0, "p",     "progress",  Arg::None,
                                                             "  -p  \t--progress\t"
                                                             "Display progress indicator"},

        {OUTPUT_PATH,      0, "o",     "output",    Arg::Required,
                                                             "  -o  \t--output <dir>\t"
                                                             "Filename to save as, default is output.smt"},
        {OUTPUT_OVERWRITE, 0, "O",     "overwrite", Arg::None,
                                                             "  -O  \t--overwrite\t"
                                                             "Overwrite files with the same output name"},

        {IMAGESIZE,        0, "i",     "imagesize", Arg::Required,
                                                             "  -i  \t--imagesize=XxY\t"
                                                             "Scale the constructed image to this size before splitting."},
        {TILESIZE,         0, "t",     "tilesize",  Arg::Required,
                                                             "  -t  \t--tilesize=XxY\t"
                                                             "Split the constructed image into tiles of this size."},
        {FORMAT,           0, "f",     "format",    Arg::Required,
                                                             "  -f  \t--format=[DXT1,RGBA8,USHORT]\t"
                                                             "default=DXT1, what format to put into the smt"},
        {TILEMAP,          0, "M",     "tilemap",   Arg::Required,
                                                             "  -M  \t--tilemap=<csv|smf>\t"
                                                             "Reconstruction tilemap."},

        {FILTER,           0, "e",     "filter",    Arg::Required,
                                                             "  -e  \t--filter=1,2-n\t"
                                                             "Filter input tile sources to only these values, filter syntax is in the form"
                                                             "1,2,3,n and 1-5,n-n and can be mixed, 1-300,350,400-900"},
        {OVERLAP,          0, "k",     "overlap",   Arg::Integer,
                                                             "  -k  \t--overlap=0\t"
                                                             "consider that pixel values overlap by this amount."},
        {BORDER,           0, "b",     "border",    Arg::Integer,
                                                             "  -b  \t--border=0\t"
                                                             "consider that each tile has a border of this width"},
        {DUPLI,            0, "d",     "dupli",     Arg::Required,
                                                             "  -d  \t--dupli=[None,Exact,Perceptual]\t"
                                                             "default=Exact, whether to detect and omit duplicates."},

        {SMTOUT,           0, "",      "smt",       Arg::None,
                                                             "\t--smt\t"              "Save tiles to smt file"},
        {IMGOUT,           0, "",      "img",       Arg::None,
                                                             "\t--img\t"              "Save tiles as images"},

        {0,                0, nullptr, nullptr,     nullptr, nullptr}
};

static void shutdown( int code ){
    OIIO::shutdown();
    exit( code );
}

int
main( int argc, char **argv )
{
    spdlog::set_pattern("[%l] %s:%#:%! | %v");    // == Variables ==
    bool overwrite = false;

    // source
    TileCache src_tileCache;
    TileMap src_tileMap;
    std::vector<uint32_t> src_filter;
    OIIO::ImageSpec sSpec;
    uint32_t overlap = 0;

    // output
    std::filesystem::path outFilePath = "output.smt";
    TileMap out_tileMap;
    int out_format = SMT_HCT_DXT1;
    OIIO::ImageSpec out_tileSpec( 32, 32, 4, TypeDesc::UINT8 );
    uint32_t out_img_width = 1024, out_img_height = 1024;
    std::unique_ptr<SMT> tempSMT;

    // relative intermediate size
    uint32_t rel_tile_width, rel_tile_height;

    // Option parsing
    // ==============
    bool fail = false;
    argc -= (argc > 0); argv += (argc > 0);
    option::Stats stats( usage, argc, argv );
    auto* options = new option::Option[ stats.options_max ];
    auto* buffer = new option::Option[ stats.buffer_max ];
    option::Parser parse( usage, argc, argv, options, buffer );

    if( options[ HELP ] || argc == 0 ) {
        int columns = getenv( "COLUMNS" ) ? atoi( getenv( "COLUMNS" ) ) : 80;
        option::printUsage( std::cout, usage, columns );
        shutdown( options[ HELP ] ? 0 : 1 );
    }

    // setup logging level
    spdlog::set_level(spdlog::level::warn);
    if( options[ VERBOSE ] )
        spdlog::set_level(spdlog::level::info);
    if( options[ QUIET ] )
        spdlog::set_level(spdlog::level::off);

    // unknown options
    for( option::Option* opt = options[ UNKNOWN ]; opt; opt = opt->next() ){
        SPDLOG_WARN( "Unknown option: {}", std::string( opt->name,opt->namelen ) );
        fail = true;
    }

    // non options
    if( parse.nonOptionsCount() == 0 ){
        SPDLOG_ERROR( "Missing input tilesources" );
        fail = true;
    }

    if( options[ OUTPUT_OVERWRITE ] )overwrite = true;

    if( (options[ IMGOUT ] && options[ SMTOUT ])
        || (!options[ IMGOUT ] && !options[ SMTOUT ]) ){
        SPDLOG_ERROR( "must have one output format --img or --smt" );
        fail = true;
    }

    // * Output Format
    if(  options[ FORMAT ] ){
        if( strcmp( options[ FORMAT ].arg, "DXT1" ) == 0 ){
            out_format = 1;
            out_tileSpec.nchannels = 4;
            out_tileSpec.set_format( TypeDesc::UINT8 );
        }

        if( strcmp( options[ FORMAT ].arg, "RGBA8" ) == 0 ){
            out_format = GL_RGBA8;
            out_tileSpec.nchannels = 4;
            out_tileSpec.set_format( TypeDesc::UINT8 );
        }

        if( strcmp( options[ FORMAT ].arg, "USHORT" ) == 0 ){
            out_format = GL_UNSIGNED_SHORT;
            out_tileSpec.nchannels = 1;
            out_tileSpec.set_format( TypeDesc::UINT16 );
        }
    }

    // * Tile Size
    if( options[ TILESIZE ] ){
        std::tie( out_tileSpec.width, out_tileSpec.height )
                = valxval( options[ TILESIZE ].arg );
        if( (out_format == 1)
         && ((out_tileSpec.width % 4) || (out_tileSpec.height % 4))
          ){
            SPDLOG_ERROR( "tilesize must be a multiple of 4x4" );
            fail = true;
        }
    } else if(! options[ IMGOUT ] ){
        out_tileSpec.width = 32;
        out_tileSpec.height = 32;
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
            SPDLOG_ERROR( "imagesize must be a multiple of 4x4" );
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

    // Output File Path
    if( options[ OUTPUT_PATH ] ){
        outFilePath = options[ OUTPUT_PATH ].arg;
        //FIXME removed checking, perhaps its not needed here.
    }

    if( fail || parse.error() ){
        SPDLOG_ERROR( "Options parsing." );
        shutdown( 1 );
    }

    // == TILE CACHE ==
    for( int i = 0; i < parse.nonOptionsCount(); ++i ){
        SPDLOG_INFO( "adding {} to tilecache.", parse.nonOption( i ) );
        src_tileCache.addSource( parse.nonOption( i ) );
    }
    if( !src_tileCache.getNumTiles() ){
        SPDLOG_CRITICAL("no tiles in cache" );
        
    }
    SPDLOG_INFO( "{} tiles in cache", src_tileCache.getNumTiles() );

    // == SOURCE TILE SPEC ==
    {
        auto tempBuf = src_tileCache.getTile(0);
        if( tempBuf.initialized() ){
            sSpec.width = tempBuf.spec().width;
            sSpec.height = tempBuf.spec().height;
            sSpec.nchannels = out_tileSpec.nchannels;
            sSpec.set_format( out_tileSpec.format );
            SPDLOG_INFO( "Source Tile Size: {}x{}", sSpec.width, sSpec.height );
        } else {
            SPDLOG_CRITICAL("request for tile 0 from tilecache failed to retrieve a value");
        }
    }

    // == FILTER ==
    if( options[ FILTER ] ){
        src_filter = expandString( options[ FILTER ].arg );
        if( src_filter.empty() ) {
            SPDLOG_ERROR( "failed to interpret filter string" );
            shutdown( 1 );
        }
    }
    else {
        SPDLOG_INFO( "no filter specified, using all tiles" );
        src_filter.resize( src_tileCache.getNumTiles() );
        for( unsigned i = 0; i < src_filter.size(); ++i ) src_filter[ i ] = i;
    }

    // == Source TILEMAP ==
    // Source the tilemap, or generate it
    if( options[ TILEMAP ] ){
        // attempt to load from smf
        if( ! std::filesystem::exists( options[ TILEMAP ].arg ) ){
            SPDLOG_ERROR( "Unable to load tilemap from: {}", options[ TILEMAP ].arg );
            shutdown( 1 );
        }
        // attempt to load from smf
        auto tempSMF = SMF::open( options[ TILEMAP ].arg );
        if( tempSMF ){
            src_tileMap = tempSMF->getMap();
        } else { // attempt to load from csv
            src_tileMap.fromCSV( options[ TILEMAP ].arg );
            if( src_tileMap.width() != 0 && src_tileMap.height() != 0 ){\
                SPDLOG_INFO( "tilemap derived from csv" );
            }
        }
        // check for failure
        if( src_tileMap.width() == 0 || src_tileMap.height() == 0 ){
            SPDLOG_ERROR( "unable to open: {}", options[ TILEMAP ].arg );
            
            shutdown( 1 );
        }
    }
    else {
        //TODO allow user to specify generation technique, ie stride,
        // or aspect ratio.
        SPDLOG_INFO( "no tilemap specified, generated one instead" );
        uint32_t squareSize = std::ceil( std::sqrt( src_filter.size() ) );
        src_tileMap = TileMap( squareSize, squareSize, [squareSize](uint32_t x, uint32_t y) -> auto {
            return y * squareSize + x; } );
    }

    // == Build source TiledImage ==
    TiledImage src_tiledImage;

    src_tiledImage.setTileMap( src_tileMap );
    src_tiledImage.setTileCache( src_tileCache );
    src_tiledImage.setTileSize( sSpec.width, sSpec.height, overlap );
    auto src_imageSpec = src_tiledImage.getImageSpec();
    SPDLOG_INFO(R"(Source Tiled Image:
    Full size: {}x{}
    Tile size: {}x{}
    TileMap size: {}x{})",
                src_imageSpec.width, src_imageSpec.height,
                sSpec.width, sSpec.height,
                src_tileMap.width(), src_tileMap.height() );

    // == IMAGESIZE ==
    if(! options[ IMAGESIZE ] ){
        out_img_width = src_imageSpec.width;
        out_img_height = src_imageSpec.height;
    }
    SPDLOG_INFO( "ImageSize = {}x{}", out_img_width, out_img_height );

    // == TILESIZE ==
    // TODO if smt is specified then default to 32x32
    if(! options[ TILESIZE ] ){
        if( options[ IMGOUT ] ){
            out_tileSpec.width = (int)out_img_width;
            out_tileSpec.height = (int)out_img_height;
        }
        else {
            out_tileSpec.width = 32;
            out_tileSpec.height = 32;
        }
    }

/*    if( out_img_width % out_tileSpec.width
            || out_img_height % out_tileSpec.height ){
        SPDLOG_ERROR( "image size must be a multiple of tile size"
            << "\n\timage size: " << out_img_width << "x" << out_img_height
            << "\n\ttile size: " << out_tileSpec.width << "x" << out_tileSpec.height;
        shutdown( 1 );
    }*/

    // == prepare output Tile Map ==
    out_tileMap.setSize( out_img_width / out_tileSpec.width,
                         out_img_height / out_tileSpec.height);

    SPDLOG_INFO( R"(    Output Sizes:
    Full Size: {}x{}
    Tile Size: {}x{}
    tileMap Size: {}x{})",
                          out_img_width, out_img_height,
                          out_tileSpec.width, out_tileSpec.height,
                          out_tileMap.width(), out_tileMap.height() );

    // work out the relative tile size
    src_imageSpec = src_tiledImage.getImageSpec();
    float xratio = (float)src_imageSpec.width / (float)out_img_width;
    float yratio = (float)src_imageSpec.height / (float)out_img_height;
    SPDLOG_INFO( "Scale Ratio: {}x{}", xratio, yratio );

    rel_tile_width = out_tileSpec.width * xratio;
    rel_tile_height = out_tileSpec.height * yratio;
    SPDLOG_INFO( "Pre-scaled tile: {}x{}", rel_tile_width, rel_tile_height );

    if( options[ SMTOUT ] ){
        tempSMT.reset( SMT::create( outFilePath , overwrite ) );
        if(! tempSMT ){
            SPDLOG_CRITICAL( "will not overwrite {}", outFilePath.string() );
            shutdown(1);
        }
        tempSMT->setType( out_format );
        tempSMT->setTileSize( out_tileSpec.width );
    }

    // tile hashtable for exact duplicate detection
    std::unordered_map<std::string, int> hash_map;
    int *item;
    hash_map.reserve(out_tileMap.length() );

    // == OUTPUT THE IMAGES ==
    int numTiles = 0;
    int numDupes = 0;
    OIIO::ROI roi = OIIO::ROI::All();
    OIIO::ImageBuf out_buf;
    for( uint32_t y = 0; y < out_tileMap.height(); ++y ) {
        for( uint32_t x = 0; x < out_tileMap.width(); ++x ){
            SPDLOG_INFO( "Processing split ({}x{})", x, y );

            roi.xbegin = x * rel_tile_width;
            roi.xend   = x * rel_tile_width + rel_tile_width;
            roi.ybegin = y * rel_tile_height;
            roi.yend   = y * rel_tile_height + rel_tile_height;
            out_buf = src_tiledImage.getRegion( roi );

            if( dupli == 1 ){
                item = &hash_map[ computePixelHashSHA1( out_buf ) ];
                if( *item ){
                    out_tileMap.setXY( x, y, *item );
                    ++numDupes;
                    continue;
                }
                else {
                    *item = numTiles;
                }
            }

            // scale according to out_tileSpec, which is conditionally defined by
            // --tilesize or --imagesize depending on whether one image is
            // being exported or whether to split up into chunks.
            out_buf = scale( out_buf , out_tileSpec );

            if( options[ SMTOUT ] ) tempSMT->append( out_buf );
            if( options[ IMGOUT ] ){
                out_buf.write( fmt::format("{}.{:0>6}.tif", outFilePath.string(), numTiles) );

            }
            out_tileMap.setXY( x, y, numTiles );
            ++numTiles;

            if( options[ PROGRESS ] ){
                progressBar( "[Progress]:",
                    out_tileMap.length() - numDupes,
                    numTiles );
            }
        }
    }
    SPDLOG_INFO( "actual:max = {}:{}", numTiles, out_tileMap.length() );
    SPDLOG_INFO( "number of dupes = {}", numDupes );

    // if the tileMap only contains 1 value, then we are only outputting
    //     a single image, so skip tileMap csv export
    if( out_tileMap.length() > 1 ){
        std::fstream out_csv(outFilePath.string() + ".csv", std::ios::out );
        out_csv << out_tileMap.csv();
        out_csv.close();
    }

    delete[] buffer;
    delete[] options;
    OIIO::shutdown();
    return 0;
}