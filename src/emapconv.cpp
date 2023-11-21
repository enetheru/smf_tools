//
// Created by nicho on 11/11/2023.
//

#include "args.h"

#include <filesystem>
#include <numeric>
#include <recoil/SMFFormat.h>
#include <spdlog/spdlog.h>

#include <tclap/CmdLine.h>
#include <tclap/fmtOutput.h>

#include <tclap/Arg.h>

//#include "smflib/oiio.h"
#include "smflib/basicio.h"
#include "smflib/smf.h"

static void shutdown( const int code ){
    //OIIO::shutdown();
    exit( code );
}

int
main( int argc, char **argv )
{

    smflib::SMF smf;
    auto &smf_h = *smf.header();

    auto heightIO = std::make_shared< smflib::BasicHeightIo >();
    //auto typeIO = std::make_shared<TypeIO>();
    smf.setHeightIO( heightIO );
    //smf.setTypeIO( typeIO );

    ValueArg<int>::Visitor updateFormats = [&]( const ValueArg<int>& arg ) {
        if( arg.getName() == "mapx" ) smf.setMapx( arg.getValue() * 128 );
        if( arg.getName() == "mapy" ) smf.setMapy( arg.getValue() * 128 );
    };
    TCLAP::fmtOutput myout;
    spdlog::set_pattern("[%l] %s:%#:%! | %v");    // Option parsing

    TCLAP::CmdLine cmd("Command description message", ' ', "0.9" );
    auto argVersion = cmd.create< SwitchArg >( "V", "version", "Displays version information and exits" );
    auto argHelp    = cmd.create< SwitchArg >( "h", "help", "Displays usage information and exits" );
    auto argQuiet   = cmd.create< SwitchArg >( "q", "quiet", "Supress Output" );
    auto argVerbose = cmd.create< MultiSwitchArg >( "v", "verbose", "MOAR output, multiple times increases output, eg; '-vvvv'" );
    auto argWerror = cmd.create< SwitchArg >( "", "werror", "treat warnings as errors and abort" );

    // Primary modes
    auto mode = cmd.create<NamedGroup>( "Program Modes(default is to display info)" );
    auto argCompile   = mode->create< SwitchArg >( "", "compile", "construct smf, smt, or any other files based on options" );
    auto argDecompile = mode->create< SwitchArg >( "", "decompile", "deconstruct input files into human readable formats" );

    // Compile SubOptions - Metadata
    auto compileMeta = cmd.create<NamedGroup>("Compile Sub-Options" );
    auto argPrefix    = compileMeta->create< ValueArg< std::filesystem::path > >( "", "prefix", "File output prefix when saving files" );
    auto argOverwrite = compileMeta->create< SwitchArg >( "", "overwrite", "overwrite existing files" );
    auto mapSizeConstraint = std::make_shared<RangeConstraint<int>>( 1, 128 );
    compileMeta->create< ValueArg< int > >( "", "mapx", "Width of map", //FIXME, a better description
        mapSizeConstraint, updateFormats );
    compileMeta->create< ValueArg< int > >( "", "mapy", "Height of map",//FIXME, a better description
        mapSizeConstraint, updateFormats );
    auto mapHeightConstraint = std::make_shared<RangeConstraint<float>>( -8129, 8192  );
    auto argMinHeight = compileMeta->create< ValueArg< float > >( "", "min-height", "In-game position of pixel value 0x00/0x0000 of height map", false, 32.0f, "float", mapHeightConstraint );
    auto argMaxHeight = compileMeta->create< ValueArg< float > >( "", "max-height", "In-game position of pixel value 0xFF/0xFFFF of height map", false, 256.0f, "float", mapHeightConstraint );


    // Compile SubOptions - Metadata Advanced
    auto compileMetaAdv = cmd.create<NamedGroup>( "Compile Advanced Sub-Options");
    auto argMapId             = compileMetaAdv->create< ValueArg< uint32_t > > ("", "map-id", "Unique ID for the map", false, static_cast<uint32_t>(0), "uint32_t");
    auto argSquare_size       = compileMetaAdv->create< ValueArg< int > > ("", "square-size", "Distance between vertices. Must be 8", false, 8, "int");
    auto argTexels_per_square = compileMetaAdv->create< ValueArg< int > > ("", "texels-per-square", "Number of texels per square, must be 8", false, 8, "int");
    auto argTile_size         = compileMetaAdv->create< ValueArg< int > > ("", "tile-size", "Number of texels in a tile, must be 32", false, 32, "int");
    auto argCompression_type  = compileMetaAdv->create< ValueArg< int > > ("", "compression-type", "Tile compression format, must be 1, Types(1=DXT1)", false, 1, "int");

    // Compile SubOptions - data
    auto compileData = cmd.create<NamedGroup>( "Compile Data Options" );
    auto argHeight_map  = compileData->create< ValueArg< Path > > ("", "height-map", "Image file to use as the height map" );//FIXME, std::make_shared<ImageConstraint>( &smf.getHeightSpec() ) );
    auto argType_map    = compileData->create< ValueArg< Path > > ("", "type-map", "Image file to use as the type map");//FIXME, std::make_shared<ImageConstraint>( &smf.getTypeSpec() ) );
    auto argTile_map    = compileData->create< ValueArg< Path > > ("", "tile-map", "File to use as the tile map");//FIXME, false, std::filesystem::path{}, "path");
    auto argMini_map    = compileData->create< ValueArg< Path > > ("", "mini-map", "Image file to use as the mini map");//FIXME, std::make_shared<ImageConstraint>( &smf.getMiniSpec() ) );
    auto argMetal_map   = compileData->create< ValueArg< Path > > ("", "metal-map", "Image file to use as the metal map");//FIXME, std::make_shared<ImageConstraint>( &smf.getMetalSpec() ));
    //FIXME auto argFeature_map = compileData->create< ValueArg< Path > > ("", "feature-map", "image file to use as the feature map", std::make_shared<ImageConstraint>( &smf.feature_spec() ));

    // Compile SubOptions - Extra Data
    auto compileDataEx = cmd.create<NamedGroup>( "Compile Extra Data Options");
    auto argGrass_map = compileDataEx->create< ValueArg< Path > > ("", "grass-map", "Image file to use as the grass map", false, std::filesystem::path{}, "path");

    // Compile SubOptions - Conversion and Modification
    auto conversion = cmd.create<NamedGroup>( "Data Conversion Options");
    auto argDiffuse_map = conversion->create< ValueArg< Path > > ("", "diffuse-map", "Image file to use to construct the smt and tilemap", false, std::filesystem::path{}, "path");
    auto argDecal       = conversion->create< ValueArg< Path > > ("", "decal", "Image file to use as a decal", false, std::filesystem::path{}, "path");
    auto decal_value = conversion->create<ValueArg< std::string >>("", "decal-value", "Hexadecimal pixel value to detect when placing decal", false, std::string{}, "colour");
    auto height_convolv =  conversion->create<ValueArg< std::string >>("", "height-convolv", "convolution matrix to apply to height data(for smoothing)", false, std::string{}, "matrix");

    // Decompile Suboptions
    auto decompile = cmd.create<NamedGroup>( "Decompile Options");
    auto reconstruct = decompile->create<SwitchArg>("", "reconstruct", "Reconstruct the diffuse texture from the inputs");
    // FIXME TCLAP::ValueAr><int>  argScale("", "scale", "Scale the reconstructed diffuse texture to this size", false, std::pair<int, int>{}, "XxY", decompile);

    // Parse TCLAP arguments
    try {
        cmd.setOutput( &myout );
        cmd.parse( argc, argv );
    }
    catch( TCLAP::ArgException &e ){
        fmt::println(stderr, "error: {} for arg {}", e.error(), e.argId() );
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
    std::array levels SPDLOG_LEVEL_NAMES;
    if( verbose >=4 ) println( "Spdlog Level: {}", levels[spdlog::get_level()] );

    fmt::println("The map dimensions are {}x{}", smf_h.mapx, smf_h.mapy );
    //fmt::println("The height map dimensions are {}x{}", smf.getHeightSpec().width, smf.getHeightSpec().width );

    shutdown(0);
    return 0;
}