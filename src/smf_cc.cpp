#include <string>
#include <fstream>

#include <spdlog/spdlog.h>

#include "option_args.h"
#include "smflib/smt.h"
#include "smflib/smf.h"
#include "smflib/util.h"


using namespace std;

enum optionsIndex
{
    UNKNOWN,
    HELP,
    VERBOSE,
    QUIET,
    VERSION,
    OUTPUT,
    FORCE,
    MAPSIZE,
    TILESIZE,
    FLOOR,
    CEILING,
    HEIGHT, TYPE, TILEMAP, MINI, METAL, FEATURES, GRASS,
    // Compression
};

const option::Descriptor usage_short[] = {
        {UNKNOWN, 0, "",      "",      Arg::None,
                                                "USAGE: smf_cc [options] [smt1 ... smtn]\n\n"
                                                "eg. $ smf_cc -v -o mymap.smf --mapsize 8x8 --height height.tif \\ \n"
                                                "    > --mini minimap.jpeg --metal metalmap.png --grass grass.png mymap.smt \n"
                                                "\nGENERAL OPTIONS:"},
        {0,       0, nullptr, nullptr, nullptr, nullptr}
};

const option::Descriptor usage[] = {
        {UNKNOWN,  0,  "",      "",         Arg::None,
                                                           "USAGE: smf_cc [options] [smt1 ... smtn]\n\n"
                                                           "eg. $ smf_cc -v -o mymap.smf --mapsize 8x8 --height height.tif \\ \n"
                                                           "    > --mini minimap.jpeg --metal metalmap.png --grass grass.png mymap.smt \n"
                                                           "\nGENERAL OPTIONS:"},
        {{},       {}, "",      "",         Arg::None,     nullptr}, // column formatting break
        {HELP,     0,  "h",     "help",     Arg::None,     "  -h,  \t--help  \tPrint usage and exit."},
        {VERBOSE,  0,  "v",     "verbose",  Arg::None,     "  -v,  \t--verbose  \tMOAR output."},
        {QUIET,    0,  "q",     "quiet",    Arg::None,     "  -q,  \t--quiet  \tSupress Output."},
        {VERSION,  0,  "",      "version",  Arg::None,     "  -V,  \t--version  \tDisplay Version Information."},
        {{},       {}, "",      "",         Arg::None,     nullptr}, // column formatting break

        {OUTPUT,   0,  "o",     "output",   Arg::Required, "  -o,  \t--output=mymap.smf"
                                                           "\tFile to operate on, will create if it doesnt exist"},

        {FORCE,    0,  "f",     "force",    Arg::None,     "  -f,  \t--force"
                                                           "\tOverwrite existing output files"},

        {MAPSIZE,  0,  "",      "mapsize",  Arg::Required, "\t--mapsize=XxZ"
                                                           "\tWidth and length of map, in spring map units eg. '--mapsize=4x4',"
                                                           "must be multiples of two."},

        {TILESIZE, 0,  "",      "tilesize", Arg::Integer, "\t--tilesize=X"
                                                           "\tSize of tiles, in pixels eg. '--tilesize=32',"
                                                           "must be multiples of 4."},

        {FLOOR,    0,  "y",     "floor",    Arg::Integer, "  -y,  \t--floor=1.0f"
                                                           "\tMinimum height of the map."},

        {CEILING,  0,  "Y",     "ceiling",  Arg::Integer, "  -Y,  \t--ceiling=1.0f"
                                                           "\tMaximum height of the map."},

        {HEIGHT,   0,  "",      "height",   Arg::File,     "\t--height=height.tif"
                                                           "\t(x*64+1)x(y*64+1):1 UINT16 Image to use for heightmap."},

        {TYPE,     0,  "",      "type",     Arg::File,     "\t--type=type.tif"
                                                           "\t(x*32)x(y*32):1 UINT8 Image to use for typemap."},

        {TILEMAP,  0,  "",      "tilemap",  Arg::File,     "\t--tilemap=map.tif"
                                                           "\t(x*16)x(y*16):1 UINT32 Image or CSV to use for tilemap."},

        {MINI,     0,  "",      "mini",     Arg::File,     "\t--mini=mini.tif"
                                                           "\t(1024)x(1024):4 UINT8 Image to use for minimap."},

        {METAL,    0,  "",      "metal",    Arg::File,     "\t--metal=metal.tif"
                                                           "\t(x*32)x(y*32):1 UINT8 Image to use for metalmap."},

        {FEATURES, 0,  "",      "features", Arg::File,     "\t--features=list.csv"
                                                           "\tList of features with format:\n\t\tNAME,X,Y,Z,R,S"},

        {GRASS,    0,  "",      "grass",    Arg::File,     "\t--grass=grass.tif"
                                                           "\t(x*16)x(y*16):1 UINT8 Image to use for grassmap."},

        {0,        0,  nullptr, nullptr,    nullptr,       nullptr}
};

static void shutdown( int code ){
    OIIO::shutdown();
    exit( code );
}

int
main( int argc, char **argv )
{
    spdlog::set_pattern("[%l] %s:%#:%! | %v");    // == temporary/global variables
    spdlog::set_level( spdlog::level::warn );

    // Options Parsing
    // ===============
    argc -= (argc > 0); argv += (argc > 0);
    int term_columns = getenv("COLUMNS" ) ? atoi(getenv("COLUMNS" ) ) : 80;
    bool arg_fail = false;
    option::Stats stats( usage, argc, argv );
    std::vector<option::Option> options(stats.options_max);
    std::vector<option::Option> buffer(stats.options_max);
    option::Parser parse( usage, argc, argv, options.data(), buffer.data() );

    // No arguments
    if( argc == 0 ){
        option::printUsage(std::cout, usage_short, term_columns, 60, 80 );
        shutdown( 1 );
    }

    // unknown options
    for( option::Option* opt = options[ UNKNOWN ]; opt; opt = opt->next() ){
        SPDLOG_ERROR( "Unknown option: {}", std::string( opt->name,opt->namelen ) );
        arg_fail = true;
    }

    // Version Message
    if( options[ VERSION ] ) {
        fmt::println("Version Information Goes here"); //FIXME add version information
        shutdown( 0 );
    }

    // Help Message
    if( options[ HELP ] ) {
        option::printUsage(std::cout, usage, term_columns, 60, 80);
        shutdown( 0 );
    }

    // setup logging level.
    if( options[ VERBOSE ] )
        spdlog::set_level(spdlog::level::info);
    if( options[ QUIET ] )
        spdlog::set_level(spdlog::level::off);

    if( parse.error() || arg_fail ) {
        SPDLOG_ERROR("Options Parsing Error");
        shutdown(1);
    }

    // End of generic options
    // ======================

    SMF *smf = nullptr;
    SMT *smt = nullptr;
    bool force = false;
    int mapWidth = 0, mapLength = 0;
    std::filesystem::path outFilePath;
    fstream tempFile;
    int tileSize = 32;
    float mapFloor = 0.01f, mapCeiling = 1.0f;

    // -o --output filename
    if( options[ OUTPUT ] ) outFilePath = options[ OUTPUT ].arg;
    else outFilePath = "output.smf";

    // -f --force
    if( options[ FORCE ] ){
        force = true;
    }

    // --mapsize
    if( options[ MAPSIZE ] ){
        std::tie( mapWidth, mapLength ) = valxval( options[ MAPSIZE ].arg );
        if( mapWidth % 2 || mapLength % 2 ){
            SPDLOG_ERROR( "map sizes must be multiples of two" );
            arg_fail = true;
        }
    }
    if( (! mapWidth || ! mapLength) && (! options[ TILEMAP ]) ){
        //FIXME dont error here, check first if a tilefile is specified.
        SPDLOG_ERROR("--mapsize not specified");
        arg_fail = true;
    }

    //TODO add squarewidth
    //TODO add texels

    // --tilesize
    // take the tilesize from the first smt added
    if( parse.nonOptionsCount() ){
        if(! SMT::test( parse.nonOption( 0 ) ) ){
            SPDLOG_CRITICAL( "additional arguments are not smt files" );
        }
        smt = SMT::open( parse.nonOption( 0 ) );
        tileSize = smt->getTileSize();
        delete smt;
        smt = nullptr;
    }
    // take the tilesize from the arguments
    if( options[ TILESIZE ] ){
        tileSize = atoi( options[ TILESIZE ].arg );
    }
    if( tileSize % 4 ){
        SPDLOG_ERROR("tile size must be a multiple of 4");
        arg_fail = true;
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
    TileMap tileMap;
    if( options[ TILEMAP ] ){
        if( std::filesystem::exists( options[ TILEMAP ].arg ) ) {
            SPDLOG_ERROR( "Unable to load file {}", options[ TILEMAP ].arg );
            shutdown(1);
        }
        // Load from smf
        std::unique_ptr<SMF> smfTemp( SMF::open( options[ TILEMAP ].arg ) );
        if( smfTemp ){
            tileMap = smfTemp->getMap();
        } else {
            // load from csv
            tileMap.fromCSV(options[TILEMAP].arg);
        }
    }

    // Fix up map height and length to match tile source and smt files.
    if( tileMap.length() ){
        int diffuseWidth = tileSize * tileMap.width();
        int diffuseHeight = tileSize * tileMap.height();
        if( diffuseWidth % 1024 || diffuseHeight % 1024){
            SPDLOG_ERROR( "(tileMap * tileSize) % 1024 != 0,"
                "supplied arguments do not construct a valid map" );
            arg_fail = true;
        }
        mapWidth = diffuseWidth / 512;
        mapLength = diffuseHeight / 512;
        SPDLOG_INFO(
R"(Checking input dimensions
    tileMap:  {}x{}
    tileSize: {}x{}
    diffuse=  {}x{}
    mapSize=  {}x{})",
        tileMap.width(), tileMap.height(),
        tileSize, tileSize,
        diffuseWidth, diffuseHeight,
        mapWidth, mapLength );
    }

    //TODO collect feature information from the command line.

    // end option parsing
    if(arg_fail || parse.error() ){
        shutdown(1);
    }

    // == lets do it! ==
    if(! (smf = SMF::create( outFilePath, force )) ){
        SPDLOG_CRITICAL( "Unable to create: {}", outFilePath.string() );
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
        OIIO::ImageBuf heightBuf( options[ HEIGHT ].arg );
        smf->writeHeight( heightBuf );
    }
    else {
        OIIO::ImageBuf empty;
        smf->writeHeight(empty);
    }

    // type
    if( options[ TYPE ] ){
        OIIO::ImageBuf typeBuf( options[ TYPE ].arg );
        smf->writeType( typeBuf );
    }
    else {
        OIIO::ImageBuf empty;
        smf->writeType( empty );
    }

    // map header
    // map smt's
    smf->writeTileHeader();

    // tilemap
    smf->writeMap( &tileMap );

    // minimap
    if( options[ MINI ] ){
        OIIO::ImageBuf miniBuf( options[ MINI ].arg );
        smf->writeMini( miniBuf );
    }
    else {
        OIIO::ImageBuf empty;
        smf->writeMini( empty );
    }

    // metalmap
    if( options[ METAL ] ){
        OIIO::ImageBuf metalBuf( options[ METAL ].arg );
        smf->writeMetal( metalBuf );
    }
    else {
        OIIO::ImageBuf empty;
        smf->writeMetal( empty );
    }


    // features
    smf->writeFeatures();

    // grass
    if( options[ GRASS ] ){
        OIIO::ImageBuf grassBuf( options[ GRASS ].arg );
        smf->writeGrass( grassBuf );
    }

    SPDLOG_INFO( smf->json().dump(4) );
    smf->good();

    delete smf;
    OIIO::shutdown();
    return 0;
}
