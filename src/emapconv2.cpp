//
// Created by nicho on 11/11/2023.
//
#include <filesystem>
#include <numeric>
#include <utility>
#include <spdlog/spdlog.h>

#include <tclap/CmdLine.h>
#include <tclap/fmtOutput.h>

#include <tclap/Arg.h>
#include <tclap/MultiSwitchArg.h>
#include <tclap/ValueArg.h>

#include <tclap/Constraint.h>
namespace TCLAP
{
    //TODO Think about modifying the tclap source to make this redundant
    class NamedGroup final : public ArgGroup {
        std::string _name;
    public:
        NamedGroup() = default;
        explicit NamedGroup(CmdLineInterface &parser) { parser.add(*this); }
        explicit NamedGroup(CmdLineInterface &parser, std::string name ) : _name(std::move(name)) { parser.add(*this); }

        bool validate() override { return false; /* All good */ }
        [[nodiscard]] bool isExclusive() const override { return false; }
        [[nodiscard]] bool isRequired() const override { return false; }

        [[nodiscard]] std::string getName() const override { return _name; }
    };

    //TODO if we are modifying the source anywyay, We could replace the existing output with this.

    class MyConstraint final : public Constraint<int>
    {
    public:
        [[nodiscard]] std::string description() const override { return {"MyConstraint Violation"}; }
        [[nodiscard]] std::string typeDesc() const override { return {"MyConstraint - typedesc"}; }
        [[nodiscard]] bool check(const int& value) const override { return false; }
    };

    class RangeInclusive final : public Constraint<int>
    {
        int _lowerBound, _upperBound;
    public:
        RangeInclusive(const int lower_bound, const int upper_bound ) : _lowerBound(lower_bound), _upperBound(upper_bound) {}
        [[nodiscard]] std::string description() const override {
            return fmt::format("Range Violation({} -> {})", _lowerBound, _upperBound );
        }
        [[nodiscard]] std::string typeDesc() const override { return {"RangeInclusive - typeDesc"}; }
        [[nodiscard]] bool check(const int& value) const override {
            return value >= _lowerBound && value <= _upperBound;
        }
    };
}

static void shutdown( const int code ){
    //OIIO::shutdown();
    exit( code );
}

int
main( int argc, char **argv )
{
    using Path = std::filesystem::path;
    using TCLAP::ValueArg;
    using TCLAP::SwitchArg;
    using TCLAP::MultiSwitchArg;
    using TCLAP::NamedGroup;
    using TCLAP::RangeInclusive;
    using TCLAP::Visitor;

    Visitor my_visitor = [&](const TCLAP::Arg &arg) { fmt::println("MyVisitor has been visited when evaluating {}", arg.getName() ); };
    auto my_constraint = std::make_shared<RangeInclusive>( -5, 5);

    TCLAP::fmtOutput myout;
    spdlog::set_pattern("[%l] %s:%#:%! | %v");    // Option parsing

    TCLAP::CmdLine cmd("Command description message", ' ', "0.9", false);
    auto argVersion = cmd.add< SwitchArg >( "V", "version", "Displays version information and exits" );
    auto argHelp    = cmd.add< SwitchArg >( "h", "help", "Displays usage information and exits" );
    auto argQuiet   = cmd.add< SwitchArg >( "q", "quiet", "Supress Output" );
    auto argVerbose = cmd.add< MultiSwitchArg >( "v", "verbose", "MOAR output, multiple times increases output, eg; '-vvvv'" );
    auto argWerror = cmd.add< SwitchArg >( "", "werror", "treat warnings as errors and abort" );

    // Primary modes
    NamedGroup mode( cmd, "Program Modes(default is to display info)" );
    auto argCompile   = mode.add< SwitchArg >( "", "compile", "construct smf, smt, or any other files based on options" );
    auto argDecompile = mode.add< SwitchArg >( "", "decompile", "deconstruct input files into human readable formats" );

    // Compile SubOptions - Metadata
    NamedGroup compileMeta( cmd, "Compile Sub-Options" );
    auto argPrefix    = compileMeta.add< ValueArg< std::filesystem::path > >( "", "prefix", "File output prefix when saving files", false, std::filesystem::path( "./" ), "prefix" );
    auto argOverwrite = compileMeta.add< SwitchArg >( "", "overwrite", "overwrite existing files" );
    auto argMapX      = compileMeta.add< ValueArg< int > >( "", "mapx", "Width of map", false, 128, "int", my_constraint, my_visitor );
    auto argMapY      = compileMeta.add< ValueArg< int > >( "", "mapy", "Height of map", false, 128, "128-int_max % 128" );
    auto argMinHeight = compileMeta.add< ValueArg< float > >( "", "min-height", "In-game position of pixel value 0x00/0x0000 of height map", false, 32.0f, "float" );
    auto argMaxHeight = compileMeta.add< ValueArg< float > >( "", "max-height", "In-game position of pixel value 0xFF/0xFFFF of height map", false, 256.0f, "float" );


    // Compile SubOptions - Metadata Advanced
    NamedGroup compileMetaAdv(cmd, "Compile Advanced Sub-Options");
	auto argMapId             = compileMetaAdv.add< ValueArg< uint32_t > > ("", "map-id", "Unique ID for the map", false, static_cast<uint32_t>(0), "uint32_t");
	auto argSquare_size       = compileMetaAdv.add< ValueArg< int > > ("", "square-size", "Distance between vertices. Must be 8", false, 8, "int");
	auto argTexels_per_square = compileMetaAdv.add< ValueArg< int > > ("", "texels-per-square", "Number of texels per square, must be 8", false, 8, "int");
	auto argTile_size         = compileMetaAdv.add< ValueArg< int > > ("", "tile-size", "Number of texels in a tile, must be 32", false, 32, "int");
	auto argCompression_type  = compileMetaAdv.add< ValueArg< int > > ("", "compression-type", "Tile compression format, must be 1, Types(1=DXT1)", false, 1, "int");

    // Compile SubOptions - data
    NamedGroup compileData(cmd, "Compile Data Options" );
	auto argHeight_map  = compileData.add< ValueArg< Path > > ("", "height-map", "Image file to use as the height map" );
	auto argType_map    = compileData.add< ValueArg< Path > > ("", "type-map", "Image file to use as the type map", false, std::filesystem::path{}, "path");
	auto argTile_map    = compileData.add< ValueArg< Path > > ("", "tile-map", "File to use as the tile map", false, std::filesystem::path{}, "path");
	auto argMini_map    = compileData.add< ValueArg< Path > > ("", "mini-map", "Image file to use as the mini map", false, std::filesystem::path{}, "path");
	auto argMetal_map   = compileData.add< ValueArg< Path > > ("", "metal-map", "Image file to use as the metal map", false, std::filesystem::path{}, "path");
	auto argFeature_map = compileData.add< ValueArg< Path > > ("", "feature-map", "image file to use as the feature map", false, std::filesystem::path{}, "path");

    // Compile SubOptions - Extra Data
    NamedGroup compileDataEx(cmd, "Compile Extra Data Options");
	auto argGrass_map = compileDataEx.add< ValueArg< Path > > ("", "grass-map", "Image file to use as the grass map", false, std::filesystem::path{}, "path");

    // Compile SubOptions - Conversion and Modification
    NamedGroup conversion(cmd, "Data Conversion Options");
	auto argDiffuse_map = conversion.add< ValueArg< Path > > ("", "diffuse-map", "Image file to use to construct the smt and tilemap", false, std::filesystem::path{}, "path");
	auto argDecal       = conversion.add< ValueArg< Path > > ("", "decal", "Image file to use as a decal", false, std::filesystem::path{}, "path");
    auto decal_value = conversion.add<ValueArg< std::string >>("", "decal-value", "Hexadecimal pixel value to detect when placing decal", false, std::string{}, "colour");
    auto height_convolv =  conversion.add<ValueArg< std::string >>("", "height-convolv", "convolution matrix to apply to height data(for smoothing)", false, std::string{}, "matrix");

    // Decompile Suboptions
    NamedGroup decompile(cmd, "Decompile Options");
    auto reconstruct = decompile.add<SwitchArg>("", "reconstruct", "Reconstruct the diffuse texture from the inputs");
    // FIXME TCLAP::ValueArg<int>  argScale("", "scale", "Scale the reconstructed diffuse texture to this size", false, std::pair<int, int>{}, "XxY", decompile);

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
    fmt::println( "Spdlog Level: {}", static_cast<int>(spdlog::get_level()) );

    if( argMapX->isSet() )
    {
        fmt::println("The value of MapX is {}", argMapX->getValue() );
    }

    shutdown(0);
    return 0;
}