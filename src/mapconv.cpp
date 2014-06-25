// local headers
#include "smt.h"
#include "smf.h"
#include "optionparser.h"

// Built in headers
#include <cstring>
#include <string>
#include <fstream>


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

    static option::ArgStatus SMT(const option::Option& option, bool smg)
    {
        char magic[16];
        ifstream file(option.arg, ios::in | ios::binary);
        file.read(magic, 16);
        if( !strcmp(magic, "spring tilefile") )
            file.close();
            return option::ARG_OK;

        fprintf(stderr, "ERROR: '%s' is not a valid spring tile file\n", option.arg);
        return option::ARG_ILLEGAL;
    }

    static option::ArgStatus SMF(const option::Option& option, bool smg)
    {
        char magic[16];
        ifstream file(option.arg, ios::in | ios::binary);
        file.read(magic, 16);
        if( !strcmp(magic, "spring map file") )
            file.close();
            return option::ARG_OK;

        fprintf(stderr, "ERROR: '%s' is not a valid spring map file\n", option.arg);
        return option::ARG_ILLEGAL;
    }

    static option::ArgStatus Image(const option::Option& option, bool msg)
    {
        /* attempt to load image file */
        ImageInput *in = ImageInput::open (option.arg);
        if (! in) {
            fprintf(stderr, "ERROR: '%s' is not a valid image file\n", option.arg);
            return option::ARG_ILLEGAL;
        }
        in->close();
        delete in;
        return option::ARG_OK;
    }
};

enum optionsIndex {UNKNOWN, VERBOSE, HELP, QUIET, EXTRACT, SLOW_DXT1, OUTPUT,
    INPUT, WIDTH, LENGTH, FLOOR, CEILING, SMTS, TILEMAP, HEIGHT, INVERT, TYPE,
    MINI, METAL, GRASS, FEATURES };

const option::Descriptor usage[] = {
    { UNKNOWN, 0, "", "", Arg::None,
        "USAGE: mapconv [options]\n\n"
        "OPTIONS:" },
    { HELP, 0, "h", "help", Arg::None,
        "  -h,  \t--help  \tPrint usage and exit." },
    { VERBOSE, 0, "v", "verbose", Arg::None,
        "  -v,  \t--verbose  \tPrint extra information." },
    { QUIET, 0, "q", "quiet", Arg::None,
        "  -q,  \t--quiet  \tSupress output." },
    { INPUT, 0, "i", "input", Arg::SMF,
        "  -i,  \t--input  \tSMF filename to load." },
    { OUTPUT, 0, "o", "output", Arg::Required,
        "  -o,  \t--output  \tOutput prefix used when saving." },
    { EXTRACT, 0, "e", "extract", Arg::None,
        "  -e,  \t--extract  \tExtract images from loaded SMF." },
    { SLOW_DXT1, 0, "", "slow_dxt1", Arg::None,
        "\t--slow_dxt1  \tUse slower but better analytics when compressing DXT1 textures" },
    { WIDTH, 0, "x", "width", Arg::Numeric,
        "  -x,  \t--width  \tWidth of the map in spring map units, Must be multiples of two." },
    { LENGTH, 0, "z", "length", Arg::Numeric,
        "  -z,  \t--length  \tLength of the map in spring map units, Must be multiples of two." },
    { FLOOR, 0, "y", "floor", Arg::Numeric,
        "  -y,  \t--floor  \tMinimum height of the map." },
    { CEILING, 0, "Y", "ceiling", Arg::Numeric,
        "  -Y,  \t--ceiling  \tMaximum height of the map." },
    { SMTS, 0, "", "smt", Arg::SMT,
        "\t--smt  \tlist of SMT files referenced." },
    { TILEMAP, 0, "", "tilemap", Arg::Image,
        "\t--tilemap  \tImage to use for tilemap." },
    { HEIGHT, 0, "", "height", Arg::Image,
        "\t--height  \tImage to use for heightmap." },
    { INVERT, 0, "", "invert", Arg::None,
        "\t--invert \tInvert the heightmap."},
    { TYPE, 0, "", "type", Arg::Image,
        "\t--type  \tImage to use for typemap." },
    { MINI, 0, "", "mini", Arg::Image,
        "\t--mini  \tImage to use for minimap." },
    { METAL, 0, "", "metal", Arg::Image,
        "\t--metal  \tImage to use for metalmap." },
    { GRASS, 0, "", "grass", Arg::Image,
        "\t--grass  \tImage to use for grassmap." },
    { FEATURES, 0, "", "features", Arg::Required,
        "\t--features  \tList of features."},
    { UNKNOWN, 0, "", "", Arg::None,
        "\nEXAMPLES:\n"
        "  mapconv -x 8 -z 8 -y -10 -Y 256 --height height.tif --metal"
        " metal.png -smts tiles.smt -tilemap tilemap.tif -o mymap.smf\n"
        "  mapconv -i oldmap.smf -o prefix" },
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

    for( option::Option* opt = options[ UNKNOWN ]; opt; opt = opt->next() )
        std::cout << "Unknown option: " << std::string( opt->name, opt->namelen ) << "\n";

    for( int i = 0; i < parse.nonOptionsCount(); ++i )
        std::cout << "Non-option #" << i << ": " << parse.nonOption( i ) << "\n";
    if( parse.error() ) exit( 1 );

    if( options[ HELP ] || argc == 0) {
        int columns = getenv( "COLUMNS" ) ? atoi( getenv( "COLUMNS" ) ) : 80;
        option::printUsage( std::cout, usage, columns );
        exit( 1 );
    }

    bool verbose, quiet, extract, slow_dxt1, invert;
    options[VERBOSE]   ? verbose = true   : verbose = false;
    options[QUIET]     ? quiet = true     : quiet = false;
    options[EXTRACT]   ? extract = true   : extract = false;
    options[SLOW_DXT1] ? slow_dxt1 = true : slow_dxt1 = false;
    options[INVERT]    ? invert = true    : invert = false;

    vector<string> tileFiles;
    for( option::Option *opt = options[ SMTS ]; opt; opt = opt->next() )
        tileFiles.push_back( opt->arg );

    int width = 8, length = 8;
    float floor = -1, ceiling = 1;
    string outputPrefix, inputFile, tilemapFile, heightFile, typeFile, miniFile,
        metalFile, grassFile, featuresFile;

    for( int i = 0; i < parse.optionsCount(); ++i )
    {
        option::Option &opt = buffer[ i ];
        switch( opt.index() ) {
        case WIDTH:
            width = atoi( opt.arg );
            break;
        case LENGTH:
            length = atoi( opt.arg );
            break;
        case FLOOR:
            floor = atof( opt.arg );
            break;
        case CEILING:
            ceiling = atof( opt.arg );
            break;
        case INPUT:
            inputFile = opt.arg;
            break;
        case OUTPUT:
            outputPrefix = opt.arg;
            break;
        case TILEMAP:
            tilemapFile = opt.arg;
            break;
        case HEIGHT:
            heightFile = opt.arg;
            break;
        case TYPE:
            typeFile = opt.arg;
            break;
        case MINI:
            miniFile = opt.arg;
            break;
        case METAL:
            metalFile = opt.arg;
            break;
        case GRASS:
            grassFile = opt.arg;
            break;
        case FEATURES:
            featuresFile = opt.arg;
            break;
        }
    }

    delete[] options;
    delete[] buffer;

    // Globals //
    /////////////

    // Load file 
    if(! strcmp( inputFile.c_str(), "" ) ) exit(1);
    SMF smf( inputFile, verbose, quiet );
    smf.load();

    smf.slowcomp = slow_dxt1;
    smf.invert = invert;

    // decompile loaded file
    if( extract )
    {
        inputFile.erase( inputFile.size() - 4 );
        smf.setOutPrefix( inputFile );
        smf.decompileAll( 0 );
    }

    // Change attributes
    smf.setDimensions( width, length, floor, ceiling );

    if( strcmp( heightFile.c_str(),   "") ) smf.setHeightFile(   heightFile   );
    if( strcmp( typeFile.c_str(),     "") ) smf.setTypeFile(     typeFile     );
    if( strcmp( miniFile.c_str(),     "") ) smf.setMinimapFile(  miniFile     );
    if( strcmp( metalFile.c_str(),    "") ) smf.setMetalFile(    metalFile    );
    if( strcmp( tilemapFile.c_str(),  "") ) smf.setTilemapFile(  tilemapFile  );
    if( strcmp( featuresFile.c_str(), "") ) smf.setFeaturesFile( featuresFile );

    if( strcmp( grassFile.c_str(),    "") ) smf.setGrassFile(    grassFile    );
    else                                    smf.unsetGrassFile();

    for( unsigned int i = 0; i < tileFiles.size(); ++i ) smf.addTileFile( tileFiles[ i ] );
    
    //Save
    if( strcmp(outputPrefix.c_str(), "") )smf.save( outputPrefix );
    return 0;
}
