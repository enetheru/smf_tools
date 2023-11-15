//
// Created by nicho on 11/11/2023.
//
#include <filesystem>
#include <numeric>
#include <utility>
#include <spdlog/spdlog.h>
#include <tclap/CmdLine.h>
#include <tclap/Arg.h>
#include <tclap/Visitor.h>

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
    class MyOutput final : public CmdLineOutput
    {
        size_t _maxWidth{80};
        size_t _indentWidth{4};

    public:
        MyOutput() = default;
        explicit MyOutput(const size_t maxWidth, const size_t indentWidth ) : _maxWidth(maxWidth),_indentWidth(indentWidth){}

        void failure(CmdLineInterface& c, ArgException& e) override {
            fmt::println(stderr, "TCLAP Error: {}", e.what() );
            fmt::println(stderr, "\t", e.typeDescription() );
            fmt::println(stderr, "\t", e.error() );
            exit(1);
        }

        void usage(CmdLineInterface& cmd) override {
            //Scan options to get Information
            size_t columnWidths[3]{8,8,8};
            std::string groupedShortOpts;

            // Short Opts
            for( const auto arg : cmd.getArgList()) {
                if( columnWidths[1] < arg->getName().length() )columnWidths[1] = arg->getName().length();
                if( columnWidths[2] < arg->getDescription().length() )columnWidths[2] = arg->getDescription().length();

                if( arg->visibleInHelp()
                    && !arg->isRequired()
                    && !arg->isValueRequired() ) groupedShortOpts += arg->getFlag();
            }
            columnWidths[2] = _maxWidth - columnWidths[0] - columnWidths[1];
            std::string shortOpts = std::format( "{: ^{}}[{}{}]","", columnWidths[0], TCLAP_FLAGSTARTCHAR, groupedShortOpts );

            // Long opts
            std::string groupOpts{};
            for( const auto group : cmd.getArgGroups() )
            {
                groupOpts += fmt::format("\n{}:\n", group->getName() );
                std::string longOpts;
                for( const auto arg : *group )
                {
                    std::string description{};
                    for( auto word : std::ranges::split_view(arg->getDescription(), ' ' ) )
                    {
                        if( description.size() - description.find_last_of('\n') >= columnWidths[2] ) {
                            description += std::format("\n{: <{}}", "", columnWidths[0] + columnWidths[1] + _indentWidth );
                        }
                        std::ranges::copy(word.begin(), word.end(), std::back_inserter(description));
                        description += " ";
                    }
                    longOpts += std::format(
                        "{: ^{}} {: <{}} {: <{}}\n",
                        arg->getFlag().empty() ? "": "-" + arg->getFlag(), columnWidths[0],
                        "--" + arg->getName(), columnWidths[1],
                        description, columnWidths[2] );
                }
                groupOpts += longOpts;
            }


            fmt::println("{}", cmd.getMessage() );
            fmt::println("USAGE:\n{}", shortOpts );
            fmt::println("Where:\n{}", groupOpts );
        }

        void version(CmdLineInterface& c) override {
            fmt::println( "my version message: 0.1" );
        }
    };

    class MyVisitor final : public Visitor
    {
    public:
        void visit() override { fmt::println("MyVisitor has been visited"); }
        ~MyVisitor() override = default;
    };

    class MyConstraint final : public Constraint<int>
    {
    public:
        [[nodiscard]] std::string description() const override { return {"MyConstraint Violation"}; };
        [[nodiscard]] std::string shortID() const override { return {"MyConstraint - Short ID"}; };
        [[nodiscard]] bool check(const int& value) const override { return false; };
    };

    class RangeInclusive final : public Constraint<int>
    {
        int _lowerBound, _upperBound;
    public:
        RangeInclusive(const int lower_bound, const int upper_bound ) : _lowerBound(lower_bound), _upperBound(upper_bound) {}
        [[nodiscard]] std::string description() const override {
            return fmt::format("Range Violation({} -> {})", _lowerBound, _upperBound );
        };
        [[nodiscard]] std::string shortID() const override { return {"MyConstraint - Short ID"}; };
        [[nodiscard]] bool check(const int& value) const override {
            return value >= _lowerBound && value <= _upperBound;
        };
    };
}

static void shutdown( const int code ){
    //OIIO::shutdown();
    exit( code );
}




int
main( int argc, char **argv )
{
    TCLAP::Visitor *my_visitor = new TCLAP::MyVisitor(); //FIXME Visitors are dumb
    TCLAP::Constraint<int> *my_constraint = new TCLAP::RangeInclusive( -5, 5);

    TCLAP::MyOutput myout;
    spdlog::set_pattern("[%l] %s:%#:%! | %v");    // Option parsing

    TCLAP::CmdLine cmd("Command description message", ' ', "0.9", false);

    TCLAP::SwitchArg      argVersion("V", "version", "Displays version information and exits", cmd);
    TCLAP::SwitchArg      argHelp("h", "help", "Displays usage information and exits", cmd);
    TCLAP::SwitchArg      argQuiet("q", "quiet", "Supress Output", cmd);
    TCLAP::MultiSwitchArg argVerbose("v", "verbose", "MOAR output, multiple times increases output, eg; '-vvvv'", cmd);
    TCLAP::SwitchArg argWerror("", "werror", "treat warnings as errors and abort", cmd );

    // Primary modes
    TCLAP::NamedGroup  mode(cmd, "Program Modes(default is to display info)");
    TCLAP::SwitchArg argCompile("", "compile", "construct smf, smt, or any other files based on options", mode);
    TCLAP::SwitchArg argDecompile("", "decompile", "deconstruct input files into human readable formats", mode);

    // Compile SubOptions - Metadata
    TCLAP::NamedGroup compileMeta(cmd, "Compile Sub-Options");
    TCLAP::ValueArg  argPrefix("", "prefix", "File output prefix when saving files", false, std::filesystem::path("./"), "prefix", compileMeta);
    TCLAP::SwitchArg argOverwrite("", "overwrite", "overwrite existing files", compileMeta);
    //TCLAP::ValueArg  argMapX("", "mapx", "Width of map", false, 128, "typedesc", compileMeta, my_visitor  );
    TCLAP::ValueArg  argMapX("", "mapx", "Width of map", false, 128, my_constraint, cmd, my_visitor  );
    TCLAP::ValueArg  argMapY("", "mapy", "Height of map", false, 128, "128-int_max % 128", compileMeta);
    TCLAP::ValueArg  argMinHeight("", "min-height", "In-game position of pixel value 0x00/0x0000 of height map", false, 32.0f, "float", compileMeta);
    TCLAP::ValueArg  argMaxHeight("", "max-height", "In-game position of pixel value 0xFF/0xFFFF of height map", false, 256.0f, "float", compileMeta);

    // Compile SubOptions - Metadata Advanced
    TCLAP::NamedGroup compileMetaAdv(cmd, "Compile Advanced Sub-Options");
    TCLAP::ValueArg argMapId("", "map-id", "Unique ID for the map", false, static_cast<uint32_t>(0), "uint32_t", compileMetaAdv);
    TCLAP::ValueArg argSquare_size("", "square-size", "Distance between vertices. Must be 8", false, 8, "int", compileMetaAdv);
    TCLAP::ValueArg argTexels_per_square("", "texels-per-square", "Number of texels per square, must be 8", false, 8, "int", compileMetaAdv);
    TCLAP::ValueArg argTile_size("", "tile-size", "Number of texels in a tile, must be 32", false, 32, "int", compileMetaAdv);
    TCLAP::ValueArg argCompression_type("", "compression-type", "Tile compression format, must be 1, Types(1=DXT1)", false, 1, "int", compileMetaAdv);

    // Compile SubOptions - data
    TCLAP::NamedGroup compileData(cmd, "Compile Data Options" );
    TCLAP::ValueArg argHeight_map("", "height-map", "Image file to use as the height map", false, std::filesystem::path{}, "path", compileData);
    TCLAP::ValueArg argType_map("", "type-map", "Image file to use as the type map", false, std::filesystem::path{}, "path", compileData);
    TCLAP::ValueArg argTile_map("", "tile-map", "File to use as the tile map", false, std::filesystem::path{}, "path", compileData);
    TCLAP::ValueArg argMini_map("", "mini-map", "Image file to use as the mini map", false, std::filesystem::path{}, "path", compileData);
    TCLAP::ValueArg argMetal_map("", "metal-map", "Image file to use as the metal map", false, std::filesystem::path{}, "path", compileData);
    TCLAP::ValueArg argFeature_map("", "feature-map", "image file to use as the feature map", false, std::filesystem::path{}, "path", compileData);

    // Compile SubOptions - Extra Data
    TCLAP::NamedGroup    compileDataEx(cmd, "Compile Extra Data Options");
    TCLAP::ValueArg argGrass_map("", "grass-map", "Image file to use as the grass map", false, std::filesystem::path{}, "path", compileDataEx);

    // Compile SubOptions - Conversion and Modification
    TCLAP::NamedGroup    conversion(cmd, "Data Conversion Options");
    TCLAP::ValueArg argDiffuse_map("", "diffuse-map", "Image file to use to construct the smt and tilemap", false, std::filesystem::path{}, "path", conversion);
    TCLAP::ValueArg argDecal("", "decal", "Image file to use as a decal", false, std::filesystem::path{}, "path", conversion);
    TCLAP::ValueArg decal_value("", "decal-value", "Hexadecimal pixel value to detect when placing decal", false, std::string{}, "colour", conversion);
    TCLAP::ValueArg height_convolv("", "height-convolv", "convolution matrix to apply to height data(for smoothing)", false, std::string{}, "matrix", conversion);

    // Decompile Suboptions
    TCLAP::NamedGroup     decompile(cmd, "Decompile Options");
    TCLAP::SwitchArg reconstruct("", "reconstruct", "Reconstruct the diffuse texture from the inputs", decompile);
    // FIXME TCLAP::ValueArg  argScale("", "scale", "Scale the reconstructed diffuse texture to this size", false, std::pair<int, int>{}, "XxY", decompile);

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
    if( argHelp ) {
        myout.usage(cmd);
        shutdown(0);
    }

    if( argVersion ) {
        myout.version(cmd);
        shutdown(0);
    }

    // setup logging level.
    int verbose = 3;
    if( argQuiet.getValue() ) verbose = 0;
    verbose += argVerbose.getValue();
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

    if( argMapX.isSet() )
    {
        fmt::println("The value of MapX is {}", argMapX.getValue() );
    }

    shutdown(0);
    return 0;
}