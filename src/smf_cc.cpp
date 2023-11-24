#include <spdlog/spdlog.h>
#include <tclap/CmdLine.h>
#include <tclap/fmtOutput.h>
#include <tclap/MultiSwitchArg.h>
#include <tclap/MultiValueArg.h>
#include <tclap/ValueArg.h>

#include "args.h"
#include "smflib/basicio.h"
#include "smflib/smf.h"

using namespace TCLAP;
using component = smflib::SMF::SMFComponent;

static void shutdown( const int code ){
    exit( code );
}

int
main( const int argc, char **argv )
{
    spdlog::set_pattern("[%l] %s:%#:%! | %v");    // Option parsing
    spdlog::set_level( spdlog::level::warn );

    const auto heightIO = std::make_shared< smflib::BasicHeightIo >();
    const auto typeIO   = std::make_shared<smflib::BasicTypeIO>();
    const auto miniIO   = std::make_shared<smflib::BasicMiniIO>();
    const auto metalIO   = std::make_shared<smflib::BasicMetalIO>();
    const auto grassIO   = std::make_shared<smflib::BasicGrassIO>();
    const auto tileIO   = std::make_shared<smflib::BasicTileIO>();
    const auto featureIO   = std::make_shared<smflib::BasicFeatureIO>();

    smflib::SMF smf;
    smf.setHeightIO( heightIO );
    smf.setTypeIO( typeIO );
    smf.setMiniIO( miniIO );
    smf.setMetalIO( metalIO );
    smf.setGrassIO( grassIO );
    smf.setTileIO( tileIO );
    smf.setFeatureIO( featureIO );

    CmdLine cmd("Command description message", ' ', "0.9" );
    const auto argVersion = cmd.create< SwitchArg >( "V", "version", "Displays version information and exits" );
    const auto argHelp    = cmd.create< SwitchArg >( "h", "help", "Displays usage information and exits" );
    const auto argQuiet   = cmd.create< SwitchArg >( "q", "quiet", "Supress Output" );
    const auto argVerbose = cmd.create< MultiSwitchArg >( "v", "verbose", "MOAR output, multiple times increases output, eg; '-vvvv'" );

    // Arguments specific to smf_info
    const auto argOutput = cmd.create<ValueArg<Path>>( "o","output","filename of output" ); //TODO default value
    // TODO, add an unlabeled argument specifier for output
    const auto argOverwrite = cmd.create<SwitchArg>("f", "overwrite", "overwrite existing file");

    const auto groupSize = cmd.create<NamedGroup>( "Dimensions of the Map", "description" );
    const auto argMapX = groupSize->create<ValueArg<int>>("x", "mapx", "width of map, must be multiple of two" ); // TODO default value, description
    const auto argMapY = groupSize->create<ValueArg<int>>("y", "mapy", "length of map, must be multiple of two" ); // TODO default value, description
    const auto argMapU = groupSize->create<ValueArg<int>>("u", "max-height", "" ); // TODO default value, description
    const auto argMapL = groupSize->create<ValueArg<int>>("l", "min-height", "" ); // TODO default value, description

    const auto groupFiles = cmd.create<NamedGroup>("Image File Inputs", "description");

    // TODO ValueArg argHeightFile <path> Height Image File, or raw file, or csv, or json
    const auto argHeightMap = groupFiles->create<ValueArg<Path>>( "h", "height-map", "" );
    // TODO ValueArg argTypeFile <path>
    const auto argTypetMap = groupFiles->create<ValueArg<Path>>( "t", "type-map", "" );
    // TODO ValueArg argMiniFile <path>
    const auto argMinitMap = groupFiles->create<ValueArg<Path>>( "m", "mini-map", "" );
    // TODO ValueArg argMetalFile <path>
    const auto argMetaltMap = groupFiles->create<ValueArg<Path>>( "M", "metal-map", "" );
    // TODO ValueArg argGrassFile <path> Optional
    const auto argGrassMap = groupFiles->create<ValueArg<Path>>( "g", "grass-map", "" );

    // TODO MultiValueArg argTileFile <path> specify multiple tile files
    const auto argTileFiles = groupFiles->create<ValueArg<Path>>("", "tile-file", "" );
    // TODO ValueArg argTileMapFile <path>
    const auto argTileMap = groupFiles->create<ValueArg<Path>>( "T", "tile-map", "" );
    // TODO ValueArg argFeatureFile <path>
    const auto argFeatures = groupFiles->create<ValueArg<Path>>( "f", "feature-file", "" );


    // Parse TCLAP arguments
    fmtOutput myout;
    try {
        cmd.setOutput( &myout );
        cmd.parse( argc, argv );
    }
    catch( ArgException&e ){
        fmt::println(stderr, "error: {} for arg {}", e.error(), e.argId() );
        shutdown(1);
    }

    if( cmd.getNumMatched() == 0 ) {
        //FIXME output short help when no arguments are supplied
        SPDLOG_ERROR( "No arguments were supplied" );
        shutdown(1);
    }

    // Deal with basic arguments
    if( argHelp->isSet() ) {
        myout.usage(cmd);
        shutdown(0);
    }

    if( argVersion->isSet() ) {
        myout.version(cmd);
        shutdown(0);
    }

    // setup logging level.
    int verbose = 3;
    if( argQuiet->getValue() ) verbose = 0;
    verbose += argVerbose->getValue();
    if( verbose > 6 )verbose = 6;

    switch( verbose ){
    case 0: spdlog::set_level( spdlog::level::off );
        break;
    case 1: spdlog::set_level( spdlog::level::critical );
        break;
    case 2: spdlog::set_level( spdlog::level::err );
        break;
    case 3: spdlog::set_level( spdlog::level::warn );
        break;
    case 4: spdlog::set_level( spdlog::level::info );
        break;
    case 5: spdlog::set_level( spdlog::level::debug );
        break;
    case 6: spdlog::set_level( spdlog::level::trace );
        break;
    default: spdlog::set_level( spdlog::level::warn );
        break;
    }
    std::array levels SPDLOG_LEVEL_NAMES;
    if( verbose >=4 ) println( "Spdlog Level: {}", levels[spdlog::get_level()] );

    shutdown(0);
    return 0;
}


//         {HEIGHT,   0,  "",      "height",   Arg::File,     "\t--height=height.tif"
//                                                            "\t(x*64+1)x(y*64+1):1 UINT16 Image to use for heightmap."},
//
//         {TYPE,     0,  "",      "type",     Arg::File,     "\t--type=type.tif"
//                                                            "\t(x*32)x(y*32):1 UINT8 Image to use for typemap."},
//
//         {TILEMAP,  0,  "",      "tilemap",  Arg::File,     "\t--tilemap=map.tif"
//                                                            "\t(x*16)x(y*16):1 UINT32 Image or CSV to use for tilemap."},
//
//         {MINI,     0,  "",      "mini",     Arg::File,     "\t--mini=mini.tif"
//                                                            "\t(1024)x(1024):4 UINT8 Image to use for minimap."},
//
//         {METAL,    0,  "",      "metal",    Arg::File,     "\t--metal=metal.tif"
//                                                            "\t(x*32)x(y*32):1 UINT8 Image to use for metalmap."},
//
//         {FEATURES, 0,  "",      "features", Arg::File,     "\t--features=list.csv"
//                                                            "\tList of features with format:\n\t\tNAME,X,Y,Z,R,S"},
//
//         {GRASS,    0,  "",      "grass",    Arg::File,     "\t--grass=grass.tif"
//                                                            "\t(x*16)x(y*16):1 UINT8 Image to use for grassmap."},


//     std::shared_ptr<SMF> smf{};
//     bool force = false;
//     int mapWidth = 0, mapLength = 0;
//     std::filesystem::path outFilePath;
//     fstream tempFile;
//     int tileSize = 32;
//     float mapFloor = 0.01f, mapCeiling = 1.0f;
//
//     // -o --output filename
//     if( options[ OUTPUT ] ) outFilePath = options[ OUTPUT ].arg;
//     else outFilePath = "output.smf";
//
//     // -f --force
//     if( options[ FORCE ] ){
//         force = true;
//     }
//
//     // --mapsize
//     if( options[ MAPSIZE ] ){
//         std::tie( mapWidth, mapLength ) = valxval( options[ MAPSIZE ].arg );
//         if( mapWidth % 2 || mapLength % 2 ){
//             SPDLOG_ERROR( "map sizes must be multiples of two" );
//             arg_fail = true;
//         }
//     }
//     if( (! mapWidth || ! mapLength) && ! options[ TILEMAP ] ){
//         //FIXME dont error here, check first if a tilefile is specified.
//         SPDLOG_ERROR("--mapsize not specified");
//         arg_fail = true;
//     }
//
//     //TODO add squarewidth
//     //TODO add texels
//
//     // --tilesize
//     // take the tilesize from the first smt added
//     if( parse.nonOptionsCount() ){
//         SMT *smt;
//         if(! SMT::test( parse.nonOption( 0 ) ) ){
//             SPDLOG_CRITICAL( "additional arguments are not smt files" );
//             shutdown( 1 );
//         }
//         smt = SMT::open( parse.nonOption( 0 ) );
//         tileSize = smt->getTileSize();
//         delete smt;
//     }
//     // take the tilesize from the arguments
//     if( options[ TILESIZE ] ){
//         tileSize = atoi( options[ TILESIZE ].arg );
//     }
//     if( tileSize % 4 ){
//         SPDLOG_ERROR("tile size must be a multiple of 4");
//         arg_fail = true;
//     }
//
//     // -y --floor
//     if( options[ FLOOR ] ){
//         mapFloor = atof( options[ FLOOR ].arg );
//     }
//
//     // -Y --ceiling
//     if( options[ CEILING ] ){
//         mapCeiling = atof( options[ CEILING ].arg );
//     }
//
//     // --tilemap
//     TileMap tileMap;
//     if( options[ TILEMAP ] ){
//         if( std::filesystem::exists( options[ TILEMAP ].arg ) ) {
//             SPDLOG_ERROR( "Unable to load file {}", options[ TILEMAP ].arg );
//             shutdown(1);
//         }
//         // Load from smf
//         if( auto smfTemp = SMF::open( options[ TILEMAP ].arg ) ){
//             tileMap = smfTemp->getMap();
//         } else {
//             // load from csv
//             tileMap.fromCSV(options[TILEMAP].arg);
//         }
//     }
//
//     // Fix up map height and length to match tile source and smt files.
//     if( tileMap.length() ){
//         int diffuseWidth = tileSize * tileMap.width();
//         int diffuseHeight = tileSize * tileMap.height();
//         if( diffuseWidth % 1024 || diffuseHeight % 1024){
//             SPDLOG_ERROR( "(tileMap * tileSize) % 1024 != 0,"
//                 "supplied arguments do not construct a valid map" );
//             arg_fail = true;
//         }
//         mapWidth = diffuseWidth / 512;
//         mapLength = diffuseHeight / 512;
//         SPDLOG_INFO(
// R"(Checking input dimensions
//     tileMap:  {}x{}
//     tileSize: {}x{}
//     diffuse=  {}x{}
//     mapSize=  {}x{})",
//         tileMap.width(), tileMap.height(),
//         tileSize, tileSize,
//         diffuseWidth, diffuseHeight,
//         mapWidth, mapLength );
//     }
//
//     //TODO collect feature information from the command line.
//
//     // end option parsing
//     if(arg_fail || parse.error() ){
//         shutdown(1);
//     }
//
//     // == lets do it! ==
//     smf = SMF::create( outFilePath, force );
//     if(! smf ){
//         SPDLOG_CRITICAL( "Unable to create: {}", outFilePath.string() );
//         shutdown( 1 );
//     }
//
//     // == Information Collection ==
//     // === header information ===
//     // * width & length
//     smf->setSize( mapWidth, mapLength );
//
//     // * squareWidth
//     // * squareTexels
//
//     // * tileSize
//     smf->setTileSize( tileSize );
//
//     // * floor & ceiling
//     smf->setDepth( mapFloor, mapCeiling );
//
//     // === extra header information ===
//     // * enable grass
//     if( options[ GRASS ] ){
//         smf->enableGrass(true);
//     }
//
//     // map header and smt files
//     // * add smt files
//     for( int i = 0; i < parse.nonOptionsCount(); ++i ){
//         smf->addTileFile( parse.nonOption( i ) );
//     }
//
//     // features
//     // * add features
//     if( options[ FEATURES ] ){
//         smf->addFeatures( options[ FEATURES ].arg );
//     }
//
//     // == calculate remaining file properties ==
//     smf->updateSpecs();
//     smf->updatePtrs();
//
//     // == Write ==
//     // header
//     smf->writeHeader();
//
//     // extra headers
//     smf->writeExtraHeaders();
//
//     // height
//     if( options[ HEIGHT ] ){
//         OIIO::ImageBuf heightBuf( options[ HEIGHT ].arg );
//         smf->writeHeight( heightBuf );
//     }
//     else {
//         OIIO::ImageBuf empty;
//         smf->writeHeight(empty);
//     }
//
//     // type
//     if( options[ TYPE ] ){
//         OIIO::ImageBuf typeBuf( options[ TYPE ].arg );
//         smf->writeType( typeBuf );
//     }
//     else {
//         OIIO::ImageBuf empty;
//         smf->writeType( empty );
//     }
//
//     // map header
//     // map smt's
//     smf->writeTileHeader();
//
//     // tilemap
//     smf->writeMap( &tileMap );
//
//     // minimap
//     if( options[ MINI ] ){
//         OIIO::ImageBuf miniBuf( options[ MINI ].arg );
//         smf->writeMini( miniBuf );
//     }
//     else {
//         OIIO::ImageBuf empty;
//         smf->writeMini( empty );
//     }
//
//     // metalmap
//     if( options[ METAL ] ){
//         OIIO::ImageBuf metalBuf( options[ METAL ].arg );
//         smf->writeMetal( metalBuf );
//     }
//     else {
//         OIIO::ImageBuf empty;
//         smf->writeMetal( empty );
//     }
//
//
//     // features
//     smf->writeFeatures();
//
//     // grass
//     if( options[ GRASS ] ){
//         OIIO::ImageBuf grassBuf( options[ GRASS ].arg );
//         smf->writeGrass( grassBuf );
//     }
//
//     SPDLOG_INFO( smf->json().dump(4) );
//     smf->good();
//
//     OIIO::shutdown();
//     return 0;
// }
