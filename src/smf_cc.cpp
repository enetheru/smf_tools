// local headers
#include "smt.h"
#include "smf.h"
#include "util.h"

#include "elog/elog.h"
#include "optionparser/optionparser.h"

#include <cstring>
#include <string>
#include <fstream>

using namespace std;
OIIO_NAMESPACE_USING;

// Argument tests //
////////////////////
struct Arg: public option::Arg
{
    static void printError(const char* msg1, const option::Option& opt, const char* msg2)
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

        if (msg) printError("Option '", option, "' requires a numeric argument\n");
        return option::ARG_ILLEGAL;
    }
};

enum optionsIndex
{
    UNKNOWN,
    VERBOSE, HELP, QUIET, 
    //File Operations
    IFILE, OVERWRITE,
    // Specification
    MAPSIZE,
    FLOOR, CEILING,
    TILESIZE,
    // Source materials
    HEIGHT, TYPE, MAP, MINI, METAL, FEATURES, GRASS,
    // Compression
    DXT1_QUALITY, 
    // Deconstruction
    EXTRACT, 
};

const option::Descriptor usage[] = {
    { UNKNOWN, 0, "", "", Arg::None,
        "USAGE: mapconv [options] [smt1 ... smtn]\n\n"
        "eg. $ mapconv -v -f mymap.smf --mapsize 8x8 --height height.tif \\ \n"
        "    > --mini minimap.jpeg --metal metalmap.png --grass grass.png mymap.smt \n"
        "\nGENERAL OPTIONS:" },
    { HELP, 0, "h", "help", Arg::None,
        "  -h,  \t--help  \tPrint usage and exit." },
    { VERBOSE, 0, "v", "verbose", Arg::None,
        "  -v,  \t--verbose  \tPrint extra information." },
    { QUIET, 0, "q", "quiet", Arg::None,
        "  -q,  \t--quiet  \tSupress output." },

    { UNKNOWN, 0, "", "", Arg::None,
        "\nFILE OPS:" },
    { IFILE, 0, "f", "file", Arg::Required,
        "  -f,  \t--file=mymap.smf  \tFile to operate on, will create if it doesnt exist" },
    { OVERWRITE, 0, "", "overwrite", Arg::Required,
        "\t--overwrite  \tOverwrite existing files" },

    { UNKNOWN, 0, "", "", Arg::None,
        "\nSPECIFICATIONS:" },
    { MAPSIZE, 0, "", "mapsize", Arg::Required,
        "\t--mapsize=XxY  \tWidth and length of map, in spring map units eg. '--mapsize=4x4', must be multiples of two." },
    { FLOOR, 0, "y", "floor", Arg::Numeric,
        "  -y,  \t--floor=1.0f  \tMinimum height of the map." },
    { CEILING, 0, "Y", "ceiling", Arg::Numeric,
        "  -Y,  \t--ceiling=1.0f  \tMaximum height of the map." },
    { TILESIZE, 0, "", "tilesize", Arg::Numeric,
        "\t--tilesize=X  \tXY resolution of tiles referenced, eg. '--tileres=32'." },

    { UNKNOWN, 0, "", "", Arg::None,
        "\nCREATION:" },
    { HEIGHT, 0, "", "height", Arg::Required,
        "\t--height=height.tif  \t(x*64+1)x(y*64+1):1 UINT16 Image to use for heightmap." },
    { TYPE, 0, "", "type", Arg::Required,
        "\t--type=type.tif  \t(x*32)x(y*32):1 UINT8 Image to use for typemap." },
    { MAP, 0, "", "map", Arg::Required,
        "\t--map=map.tif  \t(x*16)x(y*16):1 UINT32 Image to use for tilemap." },
    { MINI, 0, "", "mini", Arg::Required,
        "\t--mini=mini.tif  \t(1024)x(1024):4 UINT8 Image to use for minimap." },
    { METAL, 0, "", "metal", Arg::Required,
        "\t--metal=metal.tif  \t(x*32)x(y*32):1 UINT8 Image to use for metalmap." },
    { FEATURES, 0, "", "features", Arg::Required,
        "\t--features=list.csv  \tList of features with format:\n\t\tNAME,X,Y,Z,R,S"},
    { GRASS, 0, "", "grass", Arg::Required,
        "\t--grass=grass.tif  \t(x*16)x(y*16):1 UINT8 Image to use for grassmap." },

    { UNKNOWN, 0, "", "", Arg::None,
        "\nCOMPRESSION:" },
    { DXT1_QUALITY, 0, "", "dxt1-quality", Arg::None,
        "\t--dxt1-quality  \tUse slower but better analytics when compressing DXT1 textures" },

    { UNKNOWN, 0, "", "", Arg::None,
        "\nDECONSTRUCTION:" },
    { EXTRACT, 0, "e", "extract", Arg::None,
        "  -e,  \t--extract  \tpulls all information out of the smf in more easily understood forms, ie images and text" },

    { UNKNOWN, 0, "", "", Arg::None,
        "\nNOTES:\n"
        "Passing 'CLEAR' to the options that take a paremeter will clear the contents of that part of the file" },
    { UNKNOWN, 0, "", "", Arg::None,
        "\nEXAMPLES:\n"
        "$ mapconv -vef mymap.smf\n"
        "$ mapconv -vf mymap.smf --features CLEAR --type CLEAR" },
    {0,0,0,0,0,0}
};

int
main( int argc, char **argv )
{
    argc -= (argc > 0); argv += (argc > 0);
    option::Stats stats( usage, argc, argv );
    option::Option* options = new option::Option[ stats.options_max ];
    option::Option* buffer = new option::Option[ stats.buffer_max ];
    option::Parser parse( usage, argc, argv, options, buffer );

    bool fail = false;
    for( option::Option* opt = options[ UNKNOWN ]; opt; opt = opt->next() ){
        LOG(WARN) << "Unknown option: " << string( opt->name, opt->namelen );
        fail = true;
    }
    if( fail ) exit( 1 );
    if( parse.error() ) exit( 1 );


    if( options[ HELP ] || argc == 0 ){
        int columns = getenv( "COLUMNS" ) ? atoi( getenv( "COLUMNS" ) ) : 80;
        option::printUsage( std::cout, usage, columns );
        exit( 1 );
    }

    bool overwrite = false;

    unsigned int mx = 2, my = 2;
//    if( options[ VERBOSE   ] ) verbose = true;
//    if( options[ QUIET     ] ) quiet = true;
//    if( options[ DXT1_QUALITY ] ) dxt1_quality = true;
    if( options[ OVERWRITE ] ) overwrite = true;

    // output creation
    SMF *smf = NULL;
    string fileName;
    if( options[ IFILE ] )
        fileName = options[ IFILE ].arg;
    else
        fileName = "rename_me.smf";

    smf = SMF::open( fileName );
    if(! smf ) smf = SMF::create( fileName, overwrite );
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

    std::fstream file;
    ImageBuf *buf = NULL;
    if( options[ EXTRACT ] ){
        LOG(INFO) << "INFO: Extracting height image";
        buf = smf->getHeight();
        buf->write("height.tif", "tif" );
        LOG(INFO) << "INFO: Extracting type image";
        buf = smf->getType();
        buf->write("type.tif", "tif");

        LOG(INFO) << "INFO: Extracting map image";
        tileMap = smf->getMap();
        file.open("out_tilemap.csv", ios::out );
        file << tileMap->toCSV();
        file.close();

        LOG(INFO) << "INFO: Extracting mini image";
        buf = smf->getMini();
        buf->write("mini.tif", "tif");
        LOG(INFO) << "INFO: Extracting metal image";
        buf = smf->getMetal();
        buf->write("metal.tif", "tif");
        LOG(INFO) << "INFO: Extracting featureList";
        file.open( "featuretypes.txt", ios::out );
        file << smf->getFeatureTypes();
        file.close();
        LOG(INFO) << "INFO: Extracting features";
        file.open( "features.csv", ios::out );
        file << smf->getFeatures();
        file.close();
        buf = smf->getGrass();
        if( buf ){
            LOG(INFO) << "INFO: Extracting grass image";
            buf->write("grass.tif", "tif");
        }
    }

    LOG(INFO) << smf->info();
    return 0;
}
