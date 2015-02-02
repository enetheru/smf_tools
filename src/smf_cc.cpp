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
    INPUT,
    OUTPUT,
    FORCE,
    MAPSIZE,
    TILESIZE,
    //UNUSED: TEXELS, SQUARESIZE
    FLOOR,
    CEILING,
    HEIGHT, TYPE, MAP, MINI, METAL, FEATURES, GRASS,
    // Compression
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

    { FORCE, 0, "f", "force", Arg::None, "  -f,  \t--force"
        "\tOverwrite existing output files" },

    { MAPSIZE, 0, "", "mapsize", Arg::Required, "\t--mapsize=XxZ"
        "\tWidth and length of map, in spring map units eg. '--mapsize=4x4',"
        "must be multiples of two." },

    { TILESIZE, 0, "", "tilesize", Arg::Numeric, "\t--tilesize=X"
        "\tSize of tiles, in pixels eg. '--tilesize=32',"
        "must be multiples of 4." },

    { FLOOR, 0, "y", "floor", Arg::Numeric, "  -y,  \t--floor=1.0f"
        "\tMinimum height of the map." },

    { CEILING, 0, "Y", "ceiling", Arg::Numeric, "  -Y,  \t--ceiling=1.0f"
        "\tMaximum height of the map." },

    { HEIGHT, 0, "", "height", Arg::File, "\t--height=height.tif"
        "\t(x*64+1)x(y*64+1):1 UINT16 Image to use for heightmap." },

    { TYPE, 0, "", "type", Arg::File, "\t--type=type.tif"
        "\t(x*32)x(y*32):1 UINT8 Image to use for typemap." },

    { MAP, 0, "", "map", Arg::File, "\t--map=map.tif"
        "\t(x*16)x(y*16):1 UINT32 Image to use for tilemap." },

    { MINI, 0, "", "mini", Arg::File, "\t--mini=mini.tif"
        "\t(1024)x(1024):4 UINT8 Image to use for minimap." },

    { METAL, 0, "", "metal", Arg::File, "\t--metal=metal.tif"
        "\t(x*32)x(y*32):1 UINT8 Image to use for metalmap." },

    { FEATURES, 0, "", "features", Arg::File, "\t--features=list.csv"
        "\tList of features with format:\n\t\tNAME,X,Y,Z,R,S"},

    { GRASS, 0, "", "grass", Arg::File, "\t--grass=grass.tif"
        "\t(x*16)x(y*16):1 UINT8 Image to use for grassmap." },

    { UNKNOWN, 0, "", "", Arg::None,
        "\nNOTES:\n"
        "Passing 'CLEAR' to the options that take a paremeter will clear the"
        "contents of that part of the file" },
        
    { UNKNOWN, 0, "", "", Arg::None,
        "\nEXAMPLES:\n"
        "$ smf_cc -v -o mymap.smf --features CLEAR --type CLEAR" },
    {0,0,0,0,0,0}
};

int
main( int argc, char **argv )
{
    // == temporary/global variables
    SMF *smf = NULL;
    bool force = false;
    uint32_t mapWidth = 0, mapLength = 0;
    string outFileName;
    fstream tempFile;
    uint32_t tileSize = 32;
    float mapFloor = 0.01f, mapCeiling = 1.0f;

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
        LOG( ERROR ) << "Unknown option: " << string( opt->name, opt->namelen );
        fail = true;
    }

    //FIXME non options should be counted as smt files
    // non options
    //FIXME non options are supposed to be treated as additional smt's
    //for( int i = 1; i < parse.nonOptionsCount(); ++i ){
    //    LOG( ERROR ) << "Superflous Argument: " << parse.nonOption( i );
    //    fail = true;
    //}

    // --force
    if( options[ FORCE ] ) force = true;

    // --output filename
    if( options[ OUTPUT ] ) outFileName = options[ OUTPUT ].arg;
    else outFileName = "output.smf";

    tempFile.open( outFileName, ios::in );
    if( tempFile.good() && !force ){
        LOG( ERROR ) << outFileName << " already exists.";
        fail = true;
    }
    tempFile.close();

    // --mapsize
    if( options[ MAPSIZE ] ){
        valxval( options[ MAPSIZE ].arg, mapWidth, mapLength );
    }
    if(! mapWidth || ! mapLength){
        LOG( ERROR ) << "--mapsize not specified";
        fail = true;
    }
    if( mapWidth % 2 || mapLength % 2 ){
        LOG( ERROR ) << "map sizes must be multiples of two";
        fail = true;
    }

    // --tilesize
    if( options[ TILESIZE ] ){
        tileSize = atoi( options[ TILESIZE ].arg );
    }
    if( tileSize % 4 ){
        LOG( ERROR ) << "tile size must be a multiple of 4";
        fail = true;
    }

    // --floor
    if( options[ FLOOR ] ){
        mapFloor = atof( options[ FLOOR ].arg );
    }

    // --ceiling
    if( options[ CEILING ] ){
        mapCeiling = atof( options[ CEILING ].arg );
    }

    // end option parsing
    if( fail || parse.error() ){
        exit( 1 );
    }

    // == lets do it! ==
    smf = SMF::open( outFileName );
    if(! smf ){
        smf = SMF::create( outFileName, force );
    }

    if(! smf ){
        LOG(WARN) << "ERROR.main: unable to create " << outFileName;
        exit(1);
    }

    // Order of operation should be:
    // data collection, attributes and extras
    // write beginning to end
    //
    // == Collection ==
    // header information
    // * width
    // * length
    // * squareWidth
    // * squareTexels
    // * tileSize
    // * floor
    // * ceiling
    // extra header information
    // * enable grass
    // map header and smt files
    // * add smt files 
    // features
    // * add features
    //
    // == Write ==
    // header
    // extra headers
    // height
    // type
    // map header
    // map smt's
    // map data
    // minimap
    // metalmap
    // featuresheader
    // featuretypes
    // features
    // grass
    smf->setSize( mapWidth, mapLength );
    smf->setDepth( mapFloor, mapCeiling );
    smf->setTileSize( tileSize );

    for( int i = 0; i < parse.nonOptionsCount(); ++i ){
        smf->addTileFile( parse.nonOption( i ) );
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
    //smf->reWrite();

    LOG(INFO) << smf->info();

    delete smf;
    delete [] options;
    delete [] buffer;
    return 0;
}
