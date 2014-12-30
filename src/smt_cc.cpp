#include "smt.h"
#include "smf.h"
#include "smtool.h"
#include "util.h"
#include "tiledimage.h"

#include "optionparser/optionparser.h"
#include "elog/elog.h"

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>

#include <fstream>

using namespace std;
OIIO_NAMESPACE_USING;

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
    // General Options
    HELP, VERBOSE, QUIET,
    // File Operations
    IFILE, OVERWRITE,
    //Specification
    MAPSIZE,
    TILESIZE,
    STRIDE,
    // Creation
    DECALS,
    FILTER,
    TILEMAP,
    // Compression
    DXT1_QUALITY,
    CNUM, CPET, CNET
};

const option::Descriptor usage[] = {
    { UNKNOWN, 0, "", "", Arg::None,
        "USAGE: smt_cc [options] [source files] \n"
        "  eg. 'smt_cc -v -f mysmt.smt *.jpg'\n"
        "\nGENERAL OPTIONS:" },
    { HELP, 0, "h", "help", Arg::None,
        "  -h,  \t--help  \tPrint usage and exit." },
    { VERBOSE, 0, "v", "verbose", Arg::None,
        "  -v,  \t--verbose  \tPrint extra information." },
    { QUIET, 0, "q", "quiet", Arg::None,
        "  -q,  \t--quiet  \tSupress output." },
    { OVERWRITE, 0, "f", "force", Arg::None,
        "  -f,  \t--force  \toverwrite existing files." },

    { UNKNOWN, 0, "", "", Arg::None,
        "\nSPECIFICATIONS:" },
    { MAPSIZE, 0, "", "mapsize", Arg::Required,
        "\t--mapsize=XxY  \tWidth and length of map, in spring map units, "
           "must be multiples of two." },
    { TILESIZE, 0, "", "tilesize", Arg::Numeric,
        "\t--tilesize=X  \tXY resolution of tiles to save" },
    { STRIDE, 0, "", "stride", Arg::Numeric,
        "\t--stride=N  \tNumber of tiles horizontally" },

    { UNKNOWN, 0, "", "", Arg::None,
        "\nCREATION:" },
    { FILTER, 0, "", "filter", Arg::Required,
        "\t--filter=[1,2,n,1-n]  \tAppend only these tiles" },
    { IFILE, 0, "o", "file", Arg::Required,
        "  -f,  \t--file=filename.smt  \tfile to operate on, will be created "
            "if it doesnt exist." },

    { UNKNOWN, 0, "", "", Arg::None,
        "\nCOMPRESSION OPTIONS:" },
    { DXT1_QUALITY, 0, "", "dxt1-quality", Arg::None,
        "\t--dxt1-quality  \tUse slower but better analytics when compressing "
            "DXT1 textures" },
    { CNUM, 0, "", "cnum", Arg::Numeric,
        "\t--cnum=[-1,0,N]  \tNumber of tiles to compare; n=-1, no "
            "comparison; n=0, hashtable exact comparison; n > 0, numeric "
            "comparison between n tiles" },
    { CPET, 0, "", "cpet", Arg::Numeric,
        "\t--cpet  \tPixel error threshold. 0.0f-1.0f" },
    { CNET, 0, "", "cnet=[0.0-1.0]", Arg::Numeric,
        "\t--cnet=[0-N]  \tErrors threshold 0-1024." },

    { UNKNOWN, 0, "", "", Arg::None,
        "\nEXAMPLES:\n"
        "  smt_cc \n"
    },
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

    // setup logging level.
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
    for( int i = 1; i < parse.nonOptionsCount(); ++i ){
        LOG( WARN ) << "Superflous Argument: " << parse.nonOption( i );
        fail = true;
    }

    if( fail || parse.error() ){
        LOG( ERROR ) << "Options parsing";
        exit( 1 );
    }

    // get non bool options
    uint32_t mw,ml;
    if( options[ MAPSIZE ] ) valxval( options[ MAPSIZE ].arg, mw, ml );


    uint32_t ts;
    if( options[ TILESIZE ] ){
        ts = std::atoi( options[ TILESIZE ].arg );
        CHECK( 0 < ts && ts <= 1024 ) << "tilesize is too large, must be <= 1024";
        CHECK(! ts % 4 ) << "tilesize must be a multiple of 4";
    }
    // end of options parsing

    // Setup
    // =====
//    bool dxt1_quality = false;
//    uint32_t ix = 1024, iy = 1024;
//    uint32_t tileSize = 32;
//    if( options[ FORCE     ] ) force = true;

//    if( options[ DXT1_QUALITY ] ) dxt1_quality = true;
//    if( options[ IMAGESIZE ] ) valxval( options[ IMAGESIZE ].arg, ix, iy );

    // Firstly define the source tiled image
    // =======================================
    /* Assumptions:
        if only a single image, assume it needs to be split up into tiles
            - assume tilesize of 512 unless specified
            - assume map size is smallest square 2x2 unless specified
            - use tilesize and mapsize to determine how to scale the image
        if multiple images/sources then assume images are tiles
            - unless tilesize is given, assume tiles are the same size as the
                first source
            - assume map size is x= y= sqrt(numtiles) unless specified
    */
    /* what i want to be able to do:
        * add a single tile
        * replace a single tile
    TiledImage tiledImage;

    // Import the filenames into the source image tilecache
    for( int i = 0; i < parse.nonOptionsCount(); ++i ){
        LOG(INFO) << "Adding " << parse.nonOption( i ) << " to tileCache.";
        tiledImage.tileCache.addSource( parse.nonOption( i ) );
    }

    LOG( INFO ) << tiledImage.tileCache.getNTiles() << " tiles in cache";
    if( tiledImage.tileCache.getNTiles() == 1 ){
        LOG( WARN ) << "only one image supplied, assuming whole map.";
    }
    
    // Source the tilemap, or generate it
    if( options[ TILEMAP ] ){
        SMF *smf = NULL;
        if( (smf = SMF::open( options[ TILEMAP ].arg )) ){
            TileMap *tileMap = smf->getMap();
            tiledImage.setTileMap( *tileMap );
            delete smf;
        }
        if(! smf ){
            tiledImage.mapFromCSV( options[ TILEMAP ].arg );
        }
    }
    else {
        tiledImage.squareFromCache();
    }
*/

/*
    // load up the tilemap
    TileMap *tilemap = NULL;
    if( options[ RECONSTRUCT ] ){
        SMF *smf = NULL;
        if( (smf = SMF::open( options[ RECONSTRUCT ].arg )) ){
            tileCache.push_back( options[ RECONSTRUCT ].arg );
            tilemap = smf->getMap();
            delete smf;
        }
        if(! tilemap ){
            tilemap = new TileMap( options[ RECONSTRUCT ].arg );
        }
        if(! tilemap ){
            if(! quiet ){
                cout << "ERROR: loading tilemap failed." << endl;
            }
            exit( 1 );
        }
    }

    if( tileCache.getNTiles() == 0 ){
        if(! quiet ){
            cout << "ERROR.TileCache: no files specified for processing."
                << endl;
        }
        exit(1);
    }
    else if( tileCache.getNTiles() == 1 ){
        if( verbose ){
            cout << "INFO.TileCache: Only one file, assuming large map "
                "provided" << endl;
        }
        // tilecache assumes the size of the first tile as its size, if there's
        // only one image then we assume that we want to chop it into bits.
        // so we revert to the default value.
        tileCache.setTileSize( tileSize );
    }
    else {
        if( verbose ){
            cout << "INFO.TileCache:" << endl;
            cout << "\tFiles: " << tileCache.getNFiles() << endl;
            cout << "\tTiles: " << tileCache.getNTiles() << endl;
            cout << "\tTile Size: " << tileCache.getTileSize() << endl;
        }
    }
    tileSize = tileCache.getTileSize();

    //filter list
    vector<uint32_t> filter;
    if( options[ FILTER ] ){
        filter = expandString( options[ FILTER ].arg );
    }
    else {
        // have to generate a vector with consecutive numbering;
        filter.resize( tileCache.getNTiles() );
        for(uint32_t i = 0; i < filter.size(); ++i) filter[i] = i;
    }

    // output creation
    SMT *smt = NULL;
    if( options[ IFILE ] ){
        string fileName = options[ IFILE ].arg;
        if( (smt = SMT::open( fileName,
                        verbose, quiet,
                        dxt1_quality )) ){
            if( verbose ) cout << "INFO.smt: opened " << fileName << endl;
            tileSize = smt->getTileSize();
            tileCache.setTileSize( tileSize );
        }
        else if( (smt = SMT::create( fileName,
                        force, verbose, quiet,
                        dxt1_quality )) ){
            if( verbose ) cout << "INFO.smt: created " << fileName << endl;
            smt->setTileSize( tileSize );
        }
        else {
            if(! quiet ) cout << "ERROR.smt: unable to create " << fileName
                << endl;
            exit(1);
        }
    }

    // tileSize Override.
    // TileSize is set to be the size of the first tile loaded.
    // The existing output SMT overrides the tileSize value
    // Finally specifying it on the command line overrides the rest.
    // WARNING, if your output file has a different tileSize then all the tiles
    //will be deleted.
    if( options[ TILESIZE ] ){
        tileSize = stoi( options[ TILESIZE ].arg );
        smt->setTileSize( tileSize );
        tileCache.setTileSize( tileSize );
    } //FIXME detect map size from diffuse images

    // MapSize & Stride
    uint32_t mx = 2, my = 2;
    uint32_t hstride = 0, vstride = 0;
    if( options[ MAPSIZE ] ){
        valxval( options[ MAPSIZE ].arg, mx, my);
        hstride = mx * 512 / tileSize;
        vstride = my * 512 / tileSize;
    }
    else if( options[ STRIDE ] ){
        hstride = stoi( options[ STRIDE ].arg );
    }

    // Setup
    ImageBuf *buf = NULL;
    ImageSpec spec;
    ImageBuf fix;
    ROI iroi( 0, ix, 0, iy, 0, 1, 0, 4 );
    ROI mroi( 0, mx * 512, 0, my * 512, 0, 1, 0, 4 );

    // Process tiles Seperately
    if( options[ SEPARATE ] ){
        for( auto i = filter.begin(); i != filter.end(); ++i ){
            buf = tileCache.getTile( *i );
            if( options[ IFILE ] ){
                smt->append( buf );
            }
            else {
                char name[32];
                sprintf( name, "tile_%07i.tif", *i);
                buf->save( name, "tif");
            }
            delete buf;
        }
        exit(0);
    }

    // reconstruct / collate Tiles
    string imagename;
    if( options[ RECONSTRUCT ] ){
        buf = SMTool::reconstruct( tileCache, tilemap );
        imagename = "reconstruct.tif";
    }
    else if( options[ COLLATE ] || tileCache.getNTiles() == 1 ){
        if( tileCache.getNTiles() == 1) buf = tileCache.getOriginal( 0 );
        else buf = SMTool::collate( tileCache, hstride, vstride );
        imagename = "collate.tif";
    }
    if( buf ) fix.copy( *buf );
    else exit( 1 );

    if( options[ IFILE ] ){
        cout << "INFO: Scaling to " << mroi.xend << "x" << mroi.yend << endl;
        buf->clear();
        ImageBufAlgo::resample( *buf, fix, false, mroi );
        // process image into tiles
        SMTool::imageToSMT( smt, buf );
    }
    else {
        if( options[ IMAGESIZE ] ){
            cout << "INFO: Scaling to " << iroi.xend << "x" << iroi.yend
                << endl;
            buf->clear();
            ImageBufAlgo::resample( *buf, fix, false, iroi );
        }
        buf->save( imagename, "tif" );
        if( verbose ) cout << "INFO: Image saved as " << imagename << endl;
    }


    delete[] options;
    delete[] buffer;
    */
    exit( 0 );
}
