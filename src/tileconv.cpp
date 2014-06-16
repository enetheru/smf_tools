
#include <fstream>

#include "optionparser.h"
#include "smt.h"

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
        if(! strcmp(magic, "spring map file") ) {
            file.close();
            return option::ARG_OK;
        }

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

    static option::ArgStatus File(const option::Option& option, bool msg)
    {
        /* attempt to load image file */
        if(   (SMF(option, msg) == option::ARG_OK)
            | (Image(option, msg) == option::ARG_OK) )
            return option::ARG_OK;

        return option::ARG_ILLEGAL;
    }
};

enum optionsIndex { UNKNOWN, HELP, VERBOSE, QUIET, EXTRACT, SLOW_DXT1, CNUM, CPET,
    CNET, INPUT, OUTPUT, TILEMAP, STRIDE, WIDTH, LENGTH, DECALS, SOURCES, TILERES };

const option::Descriptor usage[] = {
    { UNKNOWN, 0, "", "", Arg::None,
        "USAGE: tileconv [options]\n\n"
        "OPTIONS:" },
    { HELP, 0, "h", "help", Arg::None,
        "  -h,  \t--help  \tPrint usage and exit." },
    { VERBOSE, 0, "v", "verbose", Arg::None,
        "  -v,  \t--verbose  \tPrint extra information." },
    { QUIET, 0, "q", "quiet", Arg::None,
        "  -q,  \t--quiet  \tSupress output." },
    { INPUT, 0, "i", "input", Arg::SMT,
        "  -i,  \t--input  \tSMT filename to load." },
    { OUTPUT, 0, "o", "output", Arg::Required,
        "  -o,  \t--output  \tOutput filename used when saving." },
    { EXTRACT, 0, "e", "extract", Arg::None,
        "  -e,  \t--extract  \tExtract images from loaded SMT." },
    { SLOW_DXT1, 0, "", "slow_dxt1", Arg::None,
        "\t--slow_dxt1  \tUse slower but better analytics when compressing DXT1 textures" },
    { WIDTH, 0, "x", "width", Arg::Numeric,
        "  -x,  \t--width  \tWidth of the map in spring map units, Must be multiples of two." },
    { LENGTH, 0, "z", "length", Arg::Numeric,
        "  -z,  \t--length  \tLength of the map in spring map units, Must be multiples of two." },
    { TILEMAP, 0, "", "tilemap", Arg::File,
        "\t--tilemap  \tImage to use for tilemap." },
    { SOURCES, 0, "", "sources", Arg::Image,
        "\t--sources  \tSource files to use for tiles" },
    { STRIDE, 0, "", "stride", Arg::Numeric,
        "\t--stride  \tNumber of image horizontally." },
    { CNUM, 0, "", "cnum", Arg::Numeric,
        "\t--cnum  \tNumber of tiles to compare; n=-1, no comparison; n=0, hashtable exact comparison; n > 0, numeric comparison between n tiles" },
    { CPET, 0, "", "cpet", Arg::Numeric,
        "\t--cpet  \tPixel error threshold. 0.0f-1.0f" },
    { CNET, 0, "", "cnet", Arg::Numeric,
        "\t--cnet  \tErrors threshold 0-1024." },
    { DECALS, 0, "", "decals", Arg::Required,
        "\t--decals  \tFile to parse when pasting decals." },
    { TILERES, 0, "", "tileres", Arg::Numeric,
        "\t--tileres  \tthe width and length of a tile." },
    { UNKNOWN, 0, "", "", Arg::None,
        "\nEXAMPLES:\n"
        "  tileconv --sources diffuse.jpg -o filename\n"
        "  tileconv -i test.smt --tilemap test_tilemap.exr  --extract"
    },
    { 0, 0, 0, 0, 0, 0 }
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
        std::cout << "Unknown option: " << std::string( opt->name,opt->namelen ) << "\n";

    for( int i = 0; i < parse.nonOptionsCount(); ++i )
        std::cout << "Non-option #" << i << ": " << parse.nonOption(i) << "\n";

    if( parse.error() ) exit( 1 );

    if( options[ HELP ] || argc == 0 ) {
        int columns = getenv( "COLUMNS" ) ? atoi( getenv( "COLUMNS" ) ) : 80;
        option::printUsage( std::cout, usage, columns );
        exit( 1 );
    }

    SMT smt;
    bool extract;

    options[ VERBOSE   ] ? smt.verbose   = true : smt.verbose   = false;
    options[ QUIET     ] ? smt.quiet     = true : smt.quiet     = false;
    options[ SLOW_DXT1 ] ? smt.slow_dxt1 = true : smt.slow_dxt1 = false;
    options[ EXTRACT   ] ? extract       = true : extract       = false;

    for( option::Option* opt = options[ SOURCES ]; opt; opt = opt->next() )
        smt.addTileSource( opt->arg );

    for( int i = 0; i < parse.optionsCount(); ++i ) {
        option::Option& opt = buffer[ i ];
        switch( opt.index() ) {
        case WIDTH:
            smt.setWidth( stoi( opt.arg ) );
            break;
        case LENGTH:
            smt.setLength( stoi( opt.arg ) );
            break;
        case CNUM:
            smt.cnum = stoi( opt.arg );
            break;
        case CNET:
            smt.cnet = stoi( opt.arg );
            break;
        case CPET:
            smt.cpet = stof( opt.arg );
            break;
        case STRIDE:
            smt.stride = stoi( opt.arg );
            break;
        case INPUT:
            smt.load( opt.arg );
            break;
        case OUTPUT:
            smt.setSaveFN( opt.arg );
            break;
        case DECALS:
            smt.setDecalFN( opt.arg );
            break;
        case TILEMAP:
            smt.setTilemapFN( opt.arg );
        case TILERES:
            smt.setTileRes( stoi(opt.arg) );
            break;
        }
    }

    delete[] options;
    delete[] buffer;
    
    // FIXME - auto detect stride
    // FIXME - auto detect map size

    if(! smt.getSaveFN().empty() ) smt.save2();

    if( extract ) smt.decompile();

    exit(0);
}
