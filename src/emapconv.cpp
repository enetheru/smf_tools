#include <spdlog/spdlog.h>
#include <optionparser.h>
#include <iostream>

#include <tclap/CmdLine.h>
#include "emapconv_args.h"
#include "recoil/SMFFormat.h"
#include "smflib/smf.h"
#include "smflib/smt.h"

SMFHeader smf_h;
TileFileHeader smt_h;
SMF smf;

int compile(option::Parser &parser, option::Option *pOption);

int decompile(option::Parser &parser, option::Option *pOption);

int info(option::Parser &parser, option::Option *pOption);

// TLMCPPOP
enum optionsIndex {
    // Basic Options
    UNKNOWN,
    VERSION,
    HELP,
    VERBOSE,
    QUIET,

    // modes:
    COMPILE,
    DECOMPILE,

    //Generic
    PREFIX, //output filename prefix
    OVERWRITE, // overwrite existing files

    // == Compile SubOptions ==
    WERROR,     // Warnings are errors
    // Metadata basic
    MAPX,
    MAPY,
    MAPZ_MIN,
    MAPZ_MAX,
    //metadata advanced
    MAP_ID,
    SQUARE_SIZE,
    TEXELS_PER_SQUARE,
    TILE_SIZE,          // used in both smf and tfh
    COMPRESSION_TYPE,   //used in TileFileHeader
    // Data
    HEIGHT_MAP,
    TYPE_MAP,
    TILE_MAP,
    MINI_MAP,
    METAL_MAP,
    FEATURE_MAP,
    // Extra Headers
    GRASS_MAP,

    // Conversion and modification
    DIFFUSE_MAP,
    DECAL,
    DECAL_VALUE,
    HEIGHT_CONVOLV,

    // == Decompile SubOptions ==
    RECONSTRUCT,
    SCALE
};

constexpr option::Descriptor usage_short[] = {
        {UNKNOWN, 0, "",      "",      Arg::None, "USAGE:\v  mapconv  \t "},
        {0,       0, nullptr, nullptr, nullptr,   nullptr}
};

constexpr option::Descriptor usage[] = {
        {UNKNOWN, 0,  "",  "",        Arg::None, "USAGE: emapconv short usage"},
        {{},      {}, "",  "",        Arg::None, nullptr}, //column formatting break
        {{},      {}, "",  "",        Arg::None, "\nOPTIONS:"},
        {VERSION, 0,  "V", "version", Arg::None, "  -V,  \t--version  \tDisplay Version Information"},
        {HELP,    0,  "h", "help",    Arg::None, "  -h,  \t--help  \tPrint usage and exit"},
        {QUIET,   0,  "q", "quiet",   Arg::None, "  -q,  \t--quiet  \tSupress Output"},
        {VERBOSE, 0,  "v", "verbose", Arg::None, "  -v,  \t--verbose  \tMOAR output, multiple times increases output, eg; '-vvvv'"},
        {{},      {}, "",  "",        Arg::None, nullptr},

        // Primary modes
        {{},        {}, "", "",          Arg::None, "\nMODES:"},
        {{},        0,  "", "",          Arg::None, "Default mode is to provide information about input options only."},
        {COMPILE,   0,  "", "compile",   Arg::None, "       \t--compile  \tconstruct smf, smt, or any other files based on options"},
        {DECOMPILE, 0,  "", "decompile", Arg::None, "       \t--decompile  \tdeconstruct input files into human readable formats"},
        {{},        {}, "", "",          Arg::None, nullptr},

        {{},        {}, "", "",          Arg::None,     "\nCOMPILE OPTIONS"},
        // Compile SubOptions - Metadata
        {{},        {}, "", "",          Arg::None,     " Metadata:"},
        {PREFIX,    0,  "", "prefix",    Arg::Required, "       \t--prefix <path>  \tFile output prefix when saving files"},
        {OVERWRITE, 0,  "", "overwrite", Arg::None,     "       \t--overwrite  \toverwrite existing files"},
        {WERROR,    0,  "", "werror",    Arg::None,     "       \t--werror  \ttreat warnings as errors and abort immediately"},
        {MAPX,      0,  "", "mapx",      Arg::MapX,     "       \t--mapx <128-8192>  \tWidth of map, Default=128"},
        {MAPY,      0,  "", "mapy",      Arg::Integer, "       \t--mapy <128-8192>  \tLength of map, Default=128"},
        {MAPZ_MIN,  0,  "", "mapz-min",  Arg::Float, "       \t--mapz-min <float>  \tIn-game position of pixel value 0x00/0x0000 of height map, Default=TODO"},
        {MAPZ_MAX,  0,  "", "mapz-max",  Arg::Float, "       \t--mapz-max <float>  \tIn-game position of pixel value 0xFF/0xFFFF of height map, Default=TODO"},
        {{},        {}, "", "",          Arg::None,     nullptr},

        // Compile SubOptions - Metadata Advanced
        {{},                {}, "", "",                 Arg::None,     "\n Advanced Metadata:"},
        {MAP_ID,            0,  "", "map-id",           Arg::Integer, "       \t--map-id <integer>  \tUnique ID for the map"},
        {SQUARE_SIZE,       0,  "", "square-size",      Arg::Integer, "       \t--square-size <integer>  \tDistance between vertices. Must be 8, Default=8"},
        {TEXELS_PER_SQUARE, 0,  "", "texels-per-square", Arg::Integer, "       \t--texels-per-square <integer>  \tNumber of texels per square, must be 8, Default=8"},
        {TILE_SIZE,         0,  "", "tile-size",        Arg::Integer, "       \t--tile-size <integer>  \tNumber of texels in a tile, must be 32, Default=32"},
        {COMPRESSION_TYPE,  0,  "", "compression-type", Arg::Integer, "       \t--compression-type <integer>  \tTile compression format, must be 1, Default=1. Types(1=DXT1)"},
        {{},                {}, "", "",                 Arg::None,     nullptr},

        // Compile SubOptions - data
        {{},          {}, "", "",            Arg::None,     "\n Input Data:"},
        {HEIGHT_MAP,  0,  "", "height-map",  Arg::Image, "       \t--height-map <path>  \tImage file to use as the height map"},
        {TYPE_MAP,    0,  "", "type-map",    Arg::Image, "       \t--type-map <path>  \tImage file to use as the type map"},
        {TILE_MAP,    0,  "", "tile-map",    Arg::File, "       \t--tile-map <path>  \tFile to use as the tile map"},
        {MINI_MAP,    0,  "", "mini-map",    Arg::Image, "       \t--mini-map <path>  \tImage file to use as the mini map"},
        {METAL_MAP,   0,  "", "metal-map",   Arg::Image, "       \t--metal-map <path>  \tImage file to use as the metal map"},
        {FEATURE_MAP, 0,  "", "feature-map", Arg::Image, "       \t--feature-map <path>  \timage file to use as the feature map"},
        {{},          {}, "", "",            Arg::None,     nullptr},

        // Compile SubOptions - Extra Data
        {{},        {}, "", "",          Arg::None,  "\n Extra Input Data:"},
        {GRASS_MAP, 0,  "", "grass-map", Arg::Image, "       \t--grass-map <path>  \tImage file to use as the grass map"},
        {{},        {}, "", "",          Arg::None,  nullptr},

        // Compile SubOptions - Conversion and Modification
        {{},          {}, "", "",            Arg::None,     "\n Conversion and modification:"},
        {DIFFUSE_MAP, 0,  "", "diffuse-map", Arg::Image, "       \t--diffuse-map <path>  \tImage file to use to construct the smt and tilemap"},
        {DECAL,       0,  "", "decal",       Arg::Image, "       \t--decal <path>  \tImage file to use as a decal"},
        {DECAL_VALUE, 0,  "", "decal-value", Arg::Required, "       \t--decal-value <hex>  \tHexadecimal pixel value to detect when placing decal"},
        {HEIGHT_CONVOLV, 0,  "", "height-convolv", Arg::Required, "       \t--height-convolv <matrix>  \tconvolution matrix to apply to height data(for smoothing)"},
        {{},          {}, "", "",            Arg::None,     nullptr},

        // Decompile Suboptions
        {{},          {}, "",      "",            Arg::None,     "\nDECOMPILE OPTIONS"},
        {PREFIX,      0,  "",      "prefix",      Arg::None,     "       \t--prefix <path>  \tFile output prefix when saving files"},
        {RECONSTRUCT, 0,  "",      "reconstruct", Arg::None,     "       \t--reconstruct  \tReconstruct the diffuse texture from the inputs"},
        {SCALE,       0,  "",      "scale",       Arg::Required, "       \t--scale <XxY>  \tScale the reconstructed diffuse texture to this size"},
        {0,           0,  nullptr, nullptr,       nullptr,       nullptr}
};

static void shutdown(const int code ){
    //OIIO::shutdown();
    exit( code );
}

int
main( int argc, char **argv )
{
    spdlog::set_pattern("[%l] %s:%#:%! | %v");    // Option parsing
    // ==============
    bool opt_fail = false;
    argc -= argc > 0; argv += argc > 0;
    const option::Stats stats( usage, argc, argv );
    auto* options = new option::Option[ stats.options_max ];
    auto* buffer = new option::Option[ stats.buffer_max ];
    option::Parser parser(usage, argc, argv, options, buffer );
    const int term_columns = getenv("COLUMNS" ) ? std::stoi(getenv("COLUMNS" ) ) : 80;

    // No arguments
    if( argc == 0 ){
        printUsage(std::cout, usage_short, term_columns );
        shutdown( 1 );
    }

    // unknown options
    for( option::Option* opt = options[ UNKNOWN ]; opt; opt = opt->next() ){
        SPDLOG_CRITICAL( "Unknown option: {}", std::string( opt->name,opt->namelen ) );
        opt_fail = true;
    }

    if( options[ VERSION ] ) {
        fmt::println("Version Information"); //TODO add version information here
        shutdown( 0 );
    }

    // Help Message
    if( options[ HELP ] ) {
        printUsage(std::cout, usage, term_columns, 60, 80 );
        shutdown( 0 );
    }

    // setup logging level.
    int verbose = 3;
    if( options[ QUIET ] ) verbose = 0;
    if( options[ VERBOSE ] ) verbose += options[ VERBOSE ].count();
    if( verbose > 6 )verbose = 6;

    switch( verbose ){
        case 0:
            spdlog::set_level(spdlog::level::off); break;
        case 1:
            spdlog::set_level(spdlog::level::critical); break;
        case 2:
            spdlog::set_level(spdlog::level::err); break;
        case 3:
            spdlog::set_level(spdlog::level::warn); break;
        case 4:
            spdlog::set_level(spdlog::level::info); break;
        case 5:
            spdlog::set_level(spdlog::level::debug); break;
        case 6:
            spdlog::set_level(spdlog::level::trace); break;
        default:
            spdlog::set_level(spdlog::level::warn); break;
    }

    if(parser.error() || opt_fail ){
        SPDLOG_CRITICAL( "Options parsing" );
        shutdown( 1 );
    }
    // end of basic options parsing

    int mode = 0; //INFO default mode
    if( options[COMPILE] )mode = 1; //COMPILE
    if( options[DECOMPILE] )mode = 2; //DECOMPILE
    if( options[COMPILE] && options[DECOMPILE] ) mode = -1; //ERROR compile and decompile are mutually exclusive
    int retcode{};
    switch( mode ){
        case 0:
            SPDLOG_INFO("INFO");
            retcode = info(parser, options );
            break;
        case 1:
            SPDLOG_INFO("COMPILE");
            retcode = compile(parser, options );
            break;
        case 2:
            SPDLOG_INFO("DECOMPILE");
            retcode = decompile(parser, options );
            break;
        case -1:
            SPDLOG_CRITICAL("--compile and --decompile are mutually exclusive");
            retcode = 1;
            break;
        default:
            SPDLOG_CRITICAL("What are we doing here?");
            retcode = 1;
    }

    //OIIO::shutdown();
    return retcode;
}

int info(option::Parser &parser, option::Option *pOption){
    int retval = 0;
    // We want to get information for the following file types
    // SMF, SMT, images
    // We want to test and provide information on arguments,
    // so if map sizes are called, we can  test images for the correct sizes

    // Compile SubOptions - Metadata
    smf_h.mapx = 128;
    if( pOption[ MAPX ] ){
        smf_h.mapx = std::stoi( pOption[MAPX].arg );
        if( smf_h.mapx < 128 ){
            SPDLOG_CRITICAL("mapx value needs to be at least 128");
            retval++;
        }
        if( smf_h.mapx > 8192 ){
            SPDLOG_WARN("mapx values larger than {} will likely crash the engine", 8192);
        }
        //TODO must be multiple of 64 or 128, cant remember.
    }
    smf_h.mapy = 128;
    if( pOption[ MAPY ] ) {
        smf_h.mapy = std::stoi(pOption[MAPY].arg);
        if( smf_h.mapy < 128 ){
            SPDLOG_CRITICAL("mapy value needs to be at least 128");
            retval++;
        }
        if( smf_h.mapy > 8192 ){
            SPDLOG_WARN("mapy values larger than {} will likely crash the engine", 8192);
        }
        //TODO must be multiple of 64 or 128, cant remember.
    }

    smf_h.minHeight = 0; //FIXME set a sane value here
    if( pOption[ MAPZ_MIN ] ) smf_h.minHeight = std::stof( pOption[ MAPZ_MIN ].arg );
    smf_h.maxHeight = 0; //FIXME set a sane value here
    if( pOption[ MAPZ_MAX ] ) smf_h.maxHeight = std::stof( pOption[ MAPZ_MAX ].arg );

    // Compile SubOptions - Metadata Advanced
    smf_h.mapid = 0;
    if( pOption[ MAP_ID ] ) {
        smf_h.mapid = std::stoi(pOption[MAP_ID].arg );
        SPDLOG_WARN("It is not recommended to set the map-id manually");
        if( pOption[WERROR] ) retval++;
    }
    smf_h.squareSize = 8;
    if( pOption[ SQUARE_SIZE ] ) {
        smf_h.squareSize = std::stoi(pOption[SQUARE_SIZE].arg);
        SPDLOG_WARN("setting the square-size to anything other than 8 will make the map invalid.");
        if( pOption[WERROR] ) retval++;
    }
    smf_h.texelPerSquare = 8;
    if( pOption[ TEXELS_PER_SQUARE ] ) {
        smf_h.texelPerSquare = std::stoi(pOption[TEXELS_PER_SQUARE].arg);
        SPDLOG_WARN( "Setting the texels-per-square to anything other than 8 will make the map invalid.");
        if( pOption[WERROR] ) retval++;
    }
    smf_h.tilesize = 32;
    smt_h.tileSize = 32;
    if( pOption[ TILE_SIZE ] ) {
        smt_h.tileSize  = smf_h.tilesize = std::stoi(pOption[TILE_SIZE].arg);
        SPDLOG_WARN( "Setting the tile-size to anything other than 32 will make the map invalid.");
        if( pOption[WERROR] ) retval++;
    }
    smt_h.compressionType = 1;
    if( pOption[ COMPRESSION_TYPE ] ) {
        smt_h.compressionType = std::stoi(pOption[COMPRESSION_TYPE].arg);
        SPDLOG_WARN( "Setting the compression-type to anything other than 1 will make the map invalid.");
        if( pOption[WERROR] ) retval++;
    }

    // Compile SubOptions - data
    if( pOption[ HEIGHT_MAP ] ) {
        auto width = smf_h.mapx + 1;
        auto height = smf_h.mapy + 1;
        std::filesystem::path filePath( pOption[HEIGHT_MAP].arg );
        if( auto image = OIIO::ImageInput::open( filePath ); image->has_error() ){
            retval++;
            SPDLOG_ERROR("Unable to open image: {}", filePath.string() );
        } else {
            auto spec = image->spec();
            if( spec.width != width ){
                    SPDLOG_WARN("height-map image width '{}' is not '{}', resizing will occur", spec.width, width);
                    if( pOption[ WERROR ] ) retval++;
            }
            if( spec.height != height ){
                SPDLOG_WARN("height-map image height'{}' is not '{}', resizing will occur", spec.height, height);
                if( pOption[ WERROR ] ) retval++;
            }
            if( spec.format != OIIO::TypeUInt16 ){
                SPDLOG_WARN("height-map image pixel format is not 16 bit, stair effect likely, consider using --height-convolv");
                if( pOption[ WERROR ] ) retval++;
            }
        }
    }
    if( pOption[ TYPE_MAP ] ){
        auto width = smf_h.mapx / 2;
        auto height = smf_h.mapy  / 2;
        std::filesystem::path filePath( pOption[TYPE_MAP].arg );
        if( auto image = OIIO::ImageInput::open( filePath ); image->has_error() ){
            retval++;
            SPDLOG_ERROR("Unable to open image: {}", filePath.string() );
        } else {
            auto spec = image->spec();
            if( spec.width != width ){
                SPDLOG_WARN("type-map image width '{}' is not '{}', resizing will occur", spec.width, width);
                if( pOption[ WERROR ] ) retval++;
            }
            if( spec.height != height ){
                SPDLOG_WARN("type-map image height'{}' is not '{}', resizing will occur", spec.height, height);
                if( pOption[ WERROR ] ) retval++;
            }
        }
    }
    if( pOption[ TILE_MAP ]) {
        //TODO
    }
    if( pOption[ MINI_MAP ]) {
        auto width = 1024;
        auto height = 1024;
        std::filesystem::path filePath( pOption[MINI_MAP].arg );
        if( auto image = OIIO::ImageInput::open( filePath ); image->has_error() ){
            retval++;
            SPDLOG_ERROR("Unable to open image: {}", filePath.string() );
        } else {
            auto spec = image->spec();
            if( spec.width != width ){
                SPDLOG_WARN("mini-map image width '{}' is not '{}', resizing will occur", spec.width, width);
                if( pOption[ WERROR ] ) retval++;
            }
            if( spec.height != height ){
                SPDLOG_WARN("mini-map image height'{}' is not '{}', resizing will occur", spec.height, height);
                if( pOption[ WERROR ] ) retval++;
            }
        }
    }
    if( pOption[ METAL_MAP ]) {
        auto width = smf_h.mapx / 2;
        auto height = smf_h.mapy  / 2;
        std::filesystem::path filePath( pOption[METAL_MAP].arg );
        if( auto image = OIIO::ImageInput::open( filePath ); image->has_error() ){
            retval++;
            SPDLOG_ERROR("Unable to open image: {}", filePath.string() );
        } else {
            auto spec = image->spec();
            if( spec.width != width ){
                SPDLOG_WARN("metal-map image width '{}' is not '{}', resizing will occur", spec.width, width);
                if( pOption[ WERROR ] ) retval++;
            }
            if( spec.height != height ){
                SPDLOG_WARN("metal-map image height'{}' is not '{}', resizing will occur", spec.height, height);
                if( pOption[ WERROR ] ) retval++;
            }
        }
    }
    if( pOption[ FEATURE_MAP ]) {
        auto width = smf_h.mapx;
        auto height = smf_h.mapy;
        std::filesystem::path filePath( pOption[FEATURE_MAP].arg );
        if( auto image = OIIO::ImageInput::open( filePath ); image->has_error() ){
            retval++;
            SPDLOG_ERROR("Unable to open image: {}", filePath.string() );
        } else {
            auto spec = image->spec();
            if( spec.width != width ){
                SPDLOG_WARN("feature-map image width '{}' is not '{}', resizing will occur", spec.width, width);
                if( pOption[ WERROR ] ) retval++;
            }
            if( spec.height != height ){
                SPDLOG_WARN("feature-map image height'{}' is not '{}', resizing will occur", spec.height, height);
                if( pOption[ WERROR ] ) retval++;
            }
        }
    }

    // Compile SubOptions - Extra Data
    if( pOption[ GRASS_MAP ]) {
        auto width = smf_h.mapx / 4;
        auto height = smf_h.mapy / 4;
        std::filesystem::path filePath( pOption[GRASS_MAP].arg );
        if( auto image = OIIO::ImageInput::open( filePath ); image->has_error() ){
            retval++;
            SPDLOG_ERROR("Unable to open image: {}", filePath.string() );
        } else {
            auto spec = image->spec();
            if( spec.width != width ){
                SPDLOG_WARN("grass-map image width '{}' is not '{}', resizing will occur", spec.width, width);
                if( pOption[ WERROR ] ) retval++;
            }
            if( spec.height != height ){
                SPDLOG_WARN("grass-map image height'{}' is not '{}', resizing will occur", spec.height, height);
                if( pOption[ WERROR ] ) retval++;
            }
        }
    }

    // Compile SubOptions - Conversion and Modification
    if( pOption[ DIFFUSE_MAP ]) {
        auto width = smf_h.mapx * 8;
        auto height = smf_h.mapy * 8;
        std::filesystem::path filePath( pOption[DIFFUSE_MAP].arg );
        if( auto image = OIIO::ImageInput::open( filePath ); image->has_error() ){
            retval++;
            SPDLOG_ERROR("Unable to open image: {}", filePath.string() );
        } else {
            auto spec = image->spec();
            if( spec.width != width ){
                SPDLOG_WARN("diffuse-map image width '{}' is not '{}', resizing will occur", spec.width, width);
                if( pOption[ WERROR ] ) retval++;
            }
            if( spec.height != height ){
                SPDLOG_WARN("diffuse-map image height'{}' is not '{}', resizing will occur", spec.height, height);
                if( pOption[ WERROR ] ) retval++;
            }
        }
    }

    if( pOption[ DECAL ]){}
    if( pOption[ DECAL_VALUE ]){}

    // Decompile Suboptions
    if( pOption[ SCALE ]){}

    // Any files specified on the command line without options associated with them can be analysed and infomration output if we can identify them
    for( int i = 0; i < parser.nonOptionsCount(); ++i ){
        SPDLOG_INFO( "Analysing: {}", parser.nonOption( i ) );
        std::filesystem::path filePath( parser.nonOption( i ) );
        if( auto [result, errMsg] = isFile( filePath.string() ); ! result ){
            SPDLOG_ERROR( errMsg );
            retval++;
            continue;
        }

        std::ifstream inFile( filePath, std::ios::binary );
        if( !inFile.is_open() ){
            SPDLOG_ERROR( "Unable to open file for reading: {}", filePath.string() );
            retval++;
            continue;
        }
        char magic[16];
        inFile.read( magic, 16 );
        // FileSize, File Permissions, etc.
        inFile.close();

        // try to open it as a spring map file
        if( std::string(magic) == "spring map file" ){
            fmt::println("This is a 'spring map file': {}", filePath.string() );
            continue;
        }

        // try to open it as a spring tile file
        if( std::string(magic) == "spring tilefile" ){
            fmt::println("This is a 'spring tilefile': {}", filePath.string() );
            continue;
        }

        // try to open it as an image file
        if( auto image = OIIO::ImageInput::open( filePath ) ){
            image->close();
            fmt::println("This is a recognisable image file: {}", filePath.string() );
        }
    }
    return retval;
}

int decompile(option::Parser &parser, option::Option *pOption) {
    return 0;
}

int compile(option::Parser &parser, option::Option *pOption) {
    if( info( parser, pOption ) ) {
        return 1;
        SPDLOG_ERROR( "Error when checking options" );
    }

    // Perform any generation first
    if( pOption[DIFFUSE_MAP] ) {
        // TODO Split into tiles
        // TODO add decals
    }

    SMF smf;
    // smf.copyHeader( smf_h );
    // smf.setHeightWriter();
    // smf.setHeightSource( OIIO::ImageBuf( pOption[ HEIGHT_MAP].arg ) );
    // smf.setTypeWriter();
    // smf.setTilesWriter();
    // smf.setMiniWriter();
    // smf.setMetalWriter();
    // smf.setFeatureWriter();
    // smf.setGrassWriter();
    // smf.setFilePath();
    // smf.write();
    return 0;
}
