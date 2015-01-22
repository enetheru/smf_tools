#include <cstring>
#include <string>
#include <fstream>

#include "elog/elog.h"
#include "optionparser/optionparser.h"

#include "option_args.h"
#include "smt.h"
#include "smf.h"
#include "util.h"

using namespace std;
OIIO_NAMESPACE_USING;

enum optionsIndex
{
    UNKNOWN,
    HELP,
    VERBOSE,
    QUIET,
    OUTPUT,
    FORCE,
    MAPSIZE,
    TILESIZE,
    //UNUSED: TEXELS, SQUARESIZE
    FLOOR,
    CEILING,
    HEIGHT, TYPE, MAP, MINI, METAL, FEATURES, GRASS,
    // Compression
    DXT1_QUALITY,
};

const option::Descriptor usage[] = {
    { UNKNOWN, 0, "", "", Arg::None,
        "USAGE: smf_cc [options] [smt1 ... smtn]\n\n"
        "eg. $ smf_cc -v -o mymap.smf --mapsize 8x8 --height height.tif \\ \n"
        "    > --mini minimap.jpeg --metal metalmap.png --grass grass.png mymap.smt \n"
        "\nGENERAL OPTIONS:" },

    { HELP, 0, "h", "help", Arg::None, "  -h,  \t--help"
        "\tPrint usage and exit." },

    { VERBOSE, 0, "v", "verbose", Arg::None, "  -v,  \t--verbose"
        "\tPrint extra information." },

    { QUIET, 0, "q", "quiet", Arg::None, "  -q,  \t--quiet"
        "\tSupress output." },

    { OUTPUT, 0, "o", "output", Arg::Required, "  -o,  \t--output=mymap.smf"
        "\tFile to operate on, will create if it doesnt exist" },

    { FORCE, 0, "f", "force", Arg::Required, "  -f,  \t--force"
        "\tOverwrite existing files" },

    { MAPSIZE, 0, "", "mapsize", Arg::Required, "\t--mapsize=XxZ"
        "\tWidth and length of map, in spring map units eg. '--mapsize=4x4',"
        "must be multiples of two." },

    { TILESIZE, 0, "", "tilesize", Arg::Required, "\t--tilesize=X"
        "\tSize of tiles, in pixels eg. '--tilesize=32',"
        "must be multiples of 4." },

    { FLOOR, 0, "y", "floor", Arg::Numeric, "  -y,  \t--floor=1.0f"
        "\tMinimum height of the map." },

    { CEILING, 0, "Y", "ceiling", Arg::Numeric, "  -Y,  \t--ceiling=1.0f"
        "\tMaximum height of the map." },

    { HEIGHT, 0, "", "height", Arg::Required, "\t--height=height.tif"
        "\t(x*64+1)x(y*64+1):1 UINT16 Image to use for heightmap." },

    { TYPE, 0, "", "type", Arg::Required, "\t--type=type.tif"
        "\t(x*32)x(y*32):1 UINT8 Image to use for typemap." },

    { MAP, 0, "", "map", Arg::Required, "\t--map=map.tif"
        "\t(x*16)x(y*16):1 UINT32 Image to use for tilemap." },

    { MINI, 0, "", "mini", Arg::Required, "\t--mini=mini.tif"
        "\t(1024)x(1024):4 UINT8 Image to use for minimap." },

    { METAL, 0, "", "metal", Arg::Required, "\t--metal=metal.tif"
        "\t(x*32)x(y*32):1 UINT8 Image to use for metalmap." },

    { FEATURES, 0, "", "features", Arg::Required, "\t--features=list.csv"
        "\tList of features with format:\n\t\tNAME,X,Y,Z,R,S"},

    { GRASS, 0, "", "grass", Arg::Required, "\t--grass=grass.tif"
        "\t(x*16)x(y*16):1 UINT8 Image to use for grassmap." },

    { DXT1_QUALITY, 0, "", "dxt1-quality", Arg::None, "\t--dxt1-quality"
        "\tUse slower but better analytics when compressing DXT1 textures" },

    { UNKNOWN, 0, "", "", Arg::None,
        "\nNOTES:\n"
        "Passing 'CLEAR' to the options that take a paremeter will clear the contents of that part of the file" },

    { UNKNOWN, 0, "", "", Arg::None,
        "\nEXAMPLES:\n"
        "$ smf_cc -v -o mymap.smf --features CLEAR --type CLEAR" },
    {0,0,0,0,0,0}
};

int
main( int argc, char **argv )
{
    // === Variables ===
    bool force = false;
    unsigned int mx = 2, my = 2; // map size
    SMF *smf = NULL; //out smf
    string fileName; //out filename

    // Options Parsing
    // ===============
    bool fail = false;
    argc -= (argc > 0); argv += (argc > 0);
    option::Stats stats( usage, argc, argv );
    option::Option* options = new option::Option[ stats.options_max ];
    option::Option* buffer = new option::Option[ stats.buffer_max ];
    option::Parser parse( usage, argc, argv, options, buffer );

    if( options[ HELP ] || argc == 0 ){
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
        LOG(WARN) << "Unknown option: " << string( opt->name, opt->namelen );
        fail = true;
    }

    //FIXME non options should be counted as smt files
    // non options
    for( int i = 1; i < parse.nonOptionsCount(); ++i ){
        LOG(WARN) << "Superflous Argument: " << parse.nonOption( i );
        fail = true;
    }

    if( fail || parse.error() ){
        LOG( ERROR ) << "Options parsing";
        exit( 1 );
    }
    // end option parsing

//    if( options[ DXT1_QUALITY ] ) dxt1_quality = true;
    if( options[ FORCE ] ) force = true;

    // output creation
    if( options[ OUTPUT ] )
        fileName = options[ OUTPUT ].arg;
    else
        fileName = "rename_me.smf";

    smf = SMF::open( fileName );
    if(! smf ) smf = SMF::create( fileName, force );
    if(! smf ){
        LOG(WARN) << "ERROR.main: unable to create " << fileName;
        exit(1);
    }

    for( int i = 0; i < parse.nonOptionsCount(); ++i ){
        smf->addTileFile( parse.nonOption( i ) );
    }

    if( options[ TILESIZE ] ){
        smf->setTileSize( stoi( options[ TILESIZE ].arg ) );
    }


    if( options[ MAPSIZE ] ){
        valxval( options[ MAPSIZE ].arg, mx, my );
        smf->setSize( mx, my );
    }

    if( options[ HEIGHT ] ){
        if(! strcmp( options[ HEIGHT ].arg, "CLEAR" ) ){
            LOG(INFO) << "INFO: Clearing Height\n";
            smf->writeHeight(NULL);
        }
        else {
            ImageBuf heightBuf( options[ HEIGHT ].arg );
            smf->writeHeight( &heightBuf );
        }
    }

    if( options[ TYPE ] ){
        if(! strcmp( options[ TYPE ].arg, "CLEAR" ) ){
            LOG(INFO) << "INFO: Clearing Type\n";
            smf->writeType(NULL);
        }
        else {
            ImageBuf typeBuf( options[ TYPE ].arg );
            smf->writeType( &typeBuf );
        }
    }

    SMF *smfTemp = NULL;
    TileMap *tileMap = NULL;
    if( options[ MAP ] ){
        if(! strcmp( options[ MAP ].arg, "CLEAR" ) ){
            LOG(INFO) << "INFO: Clearing Map\n";
            smf->writeMap(NULL);
        }
        else if( (smfTemp = SMF::open( options[ MAP ].arg )) ){
            smf->writeMap( smfTemp->getMap() );
            delete smfTemp;
        }
        else {
            tileMap = new TileMap( options[ MAP ].arg );
            smf->writeMap( tileMap );
        }
    }

    if( options[ MINI ] ){
        if(! strcmp( options[ MINI ].arg, "CLEAR" ) ){
            LOG(INFO) << "INFO: Clearing Mini\n";
            smf->writeMini(NULL);
        }
        else {
            ImageBuf miniBuf( options[ MINI ].arg );
            smf->writeMini( &miniBuf );
        }
    }

    if( options[ METAL ] ){
        if(! strcmp( options[ METAL ].arg, "CLEAR" ) ){
            LOG(INFO) << "INFO: Clearing Metal\n";
            smf->writeMetal(NULL);
        }
        else {
            ImageBuf metalBuf( options[ METAL ].arg );
            smf->writeMetal( &metalBuf );
        }
    }

    if( options[ FEATURES ] ){
        if(! strcmp( options[ FEATURES ].arg, "CLEAR" ) ){
            LOG(INFO) << "INFO: Clearing Features\n";
            smf->addFeatures("CLEAR");
        }
        else {
            smf->addFeatures( options[ FEATURES ].arg );
        }
    }

    if( options[ GRASS ] ){
        if(! strcmp( options[ GRASS ].arg, "CLEAR" ) ){
            LOG(INFO) << "INFO: Clearing Grass\n";
            smf->writeGrass(NULL);
        }
        else {
            ImageBuf grassBuf( options[ GRASS ].arg );
            smf->writeGrass( &grassBuf );
        }
    }

    /// Finalise any pending changes.
    smf->reWrite();

    LOG(INFO) << smf->info();

    delete smf;
    delete [] options;
    delete [] buffer;
    return 0;
}
