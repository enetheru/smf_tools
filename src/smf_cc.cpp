#include <string>
#include <fstream>

#include <spdlog/spdlog.h>

#include "option_args.h"
#include "smt.h"
#include "smf.h"
#include "util.h"


using namespace std;
OIIO_NAMESPACE_USING

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
    FLOOR,
    CEILING,
    HEIGHT, TYPE, TILEMAP, MINI, METAL, FEATURES, GRASS,
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

    { TILEMAP, 0, "", "tilemap", Arg::File, "\t--tilemap=map.tif"
        "\t(x*16)x(y*16):1 UINT32 Image or CSV to use for tilemap." },

    { MINI, 0, "", "mini", Arg::File, "\t--mini=mini.tif"
        "\t(1024)x(1024):4 UINT8 Image to use for minimap." },

    { METAL, 0, "", "metal", Arg::File, "\t--metal=metal.tif"
        "\t(x*32)x(y*32):1 UINT8 Image to use for metalmap." },

    { FEATURES, 0, "", "features", Arg::File, "\t--features=list.csv"
        "\tList of features with format:\n\t\tNAME,X,Y,Z,R,S"},

    { GRASS, 0, "", "grass", Arg::File, "\t--grass=grass.tif"
        "\t(x*16)x(y*16):1 UINT8 Image to use for grassmap." },

    {0,0,nullptr,nullptr,nullptr,nullptr}
};

int
main( int argc, char **argv )
{
    // == temporary/global variables
    SMF *smf = nullptr;
    SMT *smt = nullptr;
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
    auto* options = new option::Option[ stats.options_max ];
    auto* buffer = new option::Option[ stats.buffer_max ];
    option::Parser parse( usage, argc, argv, options, buffer );

    // unknown options
    for( option::Option* opt = options[ UNKNOWN ]; opt; opt = opt->next() ){
        spdlog::error( "Unknown option: {}", string( opt->name, opt->namelen ) );
        fail = true;
    }

    // -h --help
    if( options[ HELP ] || argc == 0 ){
        int columns = getenv( "COLUMNS" ) ? atoi( getenv( "COLUMNS" ) ) : 80;
        option::printUsage( std::cout, usage, columns );
        exit( 1 );
    }

    // -v --verbose
    spdlog::set_level(spdlog::level::warn);
    if( options[ VERBOSE ] )
        spdlog::set_level(spdlog::level::info);

    // -q --quiet
    if( options[ QUIET ] )
        spdlog::set_level(spdlog::level::off);

    // -o --output filename
    if( options[ OUTPUT ] ) outFileName = options[ OUTPUT ].arg;
    else outFileName = "output.smf";

    // -f --force
    if( options[ FORCE ] ){
        force = true;
    }

    // --mapsize
    if( options[ MAPSIZE ] ){
        std::tie( mapWidth, mapLength ) = valxval( options[ MAPSIZE ].arg );
        if( mapWidth % 2 || mapLength % 2 ){
            spdlog::error( "map sizes must be multiples of two" );
            fail = true;
        }
    }
    if( (! mapWidth || ! mapLength) && (! options[ TILEMAP ]) ){
        //FIXME dont error here, check first if a tilefile is specified.
        spdlog::error("--mapsize not specified");
        fail = true;
    }

    //TODO add squarewidth
    //TODO add texels

    // --tilesize
    // take the tilesize from the first smt added
    if( parse.nonOptionsCount() ){
        if(! SMT::test( parse.nonOption( 0 ) ) ){
            spdlog::critical( "additional arguments are not smt files" );
        }
        smt = SMT::open( parse.nonOption( 0 ) );
        tileSize = smt->tileSize;
        delete smt;
        smt = nullptr;
    }
    // take the tilesize from the arguments
    if( options[ TILESIZE ] ){
        tileSize = atoi( options[ TILESIZE ].arg );
    }
    if( tileSize % 4 ){
        spdlog::error("tile size must be a multiple of 4");
        fail = true;
    }

    // -y --floor
    if( options[ FLOOR ] ){
        mapFloor = atof( options[ FLOOR ].arg );
    }

    // -Y --ceiling
    if( options[ CEILING ] ){
        mapCeiling = atof( options[ CEILING ].arg );
    }

    // --tilemap
    SMF *smfTemp = nullptr;
    TileMap *tileMap = nullptr;
    if( options[ TILEMAP ] ){
        if( SMF::test( options[ TILEMAP ].arg ) ){
            smfTemp = SMF::open( options[ TILEMAP ].arg );
            tileMap = smfTemp->getMap();
            delete smfTemp;
        }
        else {
            tileMap = TileMap::createCSV( options[ TILEMAP ].arg );
        }
    }

    // Fix up map height and length to match tile source and smt files.
    if( tileMap != nullptr ){
        int diffuseWidth = tileSize * tileMap->width;
        int diffuseHeight = tileSize * tileMap->height;
        if( diffuseWidth % 1024 || diffuseHeight % 1024){
            spdlog::error( "(tileMap * tileSize) % 1024 != 0,"
                "supplied arguments do not construct a valid map" );
            fail = true;
        }
        mapWidth = diffuseWidth / 512;
        mapLength = diffuseHeight / 512;
        spdlog::info(
R"(Checking input dimensions
    tileMap:  {}x{}
    tileSize: {}x{}
    diffuse=  {}x{}
    mapSize=  {}x{})",
        tileMap->width, tileMap->height,
        tileSize, tileSize,
        diffuseWidth, diffuseHeight,
        mapWidth, mapLength );
    }

    //TODO collect feature information from the command line.

    // end option parsing
    if( fail || parse.error() ){
        exit( 1 );
    }

    // == lets do it! ==
    if(! (smf = SMF::create( outFileName, force )) ){
        spdlog::critical( "Unable to create: {}", outFileName );
    }

    // == Information Collection ==
    // === header information ===
    // * width & length
    smf->setSize( mapWidth, mapLength );

    // * squareWidth
    // * squareTexels

    // * tileSize
    smf->setTileSize( tileSize );

    // * floor & ceiling
    smf->setDepth( mapFloor, mapCeiling );

    // === extra header information ===
    // * enable grass
    if( options[ GRASS ] ){
        smf->enableGrass(true);
    }

    // map header and smt files
    // * add smt files
    for( int i = 0; i < parse.nonOptionsCount(); ++i ){
        smf->addTileFile( parse.nonOption( i ) );
    }

    // features
    // * add features
    if( options[ FEATURES ] ){
        smf->addFeatures( options[ FEATURES ].arg );
    }

    // == calculate remaining file properties ==
    smf->updateSpecs();
    smf->updatePtrs();

    // == Write ==
    // header
    smf->writeHeader();

    // extra headers
    smf->writeExtraHeaders();

    // height
    if( options[ HEIGHT ] ){
        ImageBuf heightBuf( options[ HEIGHT ].arg );
        smf->writeHeight( &heightBuf );
    }
    else {
        smf->writeHeight();
    }

    // type
    if( options[ TYPE ] ){
        ImageBuf typeBuf( options[ TYPE ].arg );
        smf->writeType( &typeBuf );
    }
    else {
        smf->writeType();
    }

    // map header
    // map smt's
    smf->writeTileHeader();

    // tilemap
    smf->writeMap( tileMap );

    // minimap
    if( options[ MINI ] ){
        ImageBuf miniBuf( options[ MINI ].arg );
        smf->writeMini( &miniBuf );
    }
    else {
        smf->writeMini();
    }

    // metalmap
    if( options[ METAL ] ){
        ImageBuf metalBuf( options[ METAL ].arg );
        smf->writeMetal( &metalBuf );
    }
    else {
        smf->writeMetal();
    }


    // features
    smf->writeFeatures();

    // grass
    if( options[ GRASS ] ){
        ImageBuf grassBuf( options[ GRASS ].arg );
        smf->writeGrass( &grassBuf );
    }

    spdlog::info( smf->info() );
    smf->good();

    delete smf;
    delete [] options;
    delete [] buffer;
    return 0;
}
