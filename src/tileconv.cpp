
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

enum optionsIndex { UNKNOWN, HELP, VERBOSE, QUIET, EXTRACT, SLOW_DXT1, CNUM, CPET,
    CNET, INPUT, OUTPUT, TILEMAP, STRIDE, WIDTH, LENGTH, DECALS, SOURCES };

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
        "  -o,  \t--output  \tOutput prefix used when saving." },
    { EXTRACT, 0, "e", "extract", Arg::None,
        "  -e,  \t--extract  \tExtract images from loaded SMT." },
    { SLOW_DXT1, 0, "", "slow_dxt1", Arg::None,
        "\t--slow_dxt1  \tUse slower but better analytics when compressing DXT1 textures" },
    { WIDTH, 0, "x", "width", Arg::Numeric,
        "  -x,  \t--width  \tWidth of the map in spring map units, Must be multiples of two." },
    { LENGTH, 0, "z", "length", Arg::Numeric,
        "  -z,  \t--length  \tLength of the map in spring map units, Must be multiples of two." },
    { TILEMAP, 0, "", "tilemap", Arg::Image,
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
    { UNKNOWN, 0, "", "", Arg::None,
        "\nEXAMPLES:\n"
        "  tileconv --sources diffuse.jpg -o filename\n"
        "  tileconv -i test.smt --tilemap test_tilemap.exr  --extract"
    },
    { 0, 0, 0, 0, 0, 0 }
};

#ifdef WIN32
int
_tmain( int argc, _TCHAR *argv[] )
#else
int
main( int argc, char **argv )
#endif
{
    argc-=(argc>0); argv+=(argc>0);
    option::Stats stats(usage, argc, argv);
    option::Option* options = new option::Option[stats.options_max];
    option::Option* buffer = new option::Option[stats.buffer_max];
    option::Parser parse(usage, argc, argv, options, buffer);

    for (option::Option* opt = options[UNKNOWN]; opt; opt = opt->next())
        std::cout << "Unknown option: " << std::string(opt->name,opt->namelen) << "\n";

    for (int i = 0; i < parse.nonOptionsCount(); ++i)
        std::cout << "Non-option #" << i << ": " << parse.nonOption(i) << "\n";
    if( parse.error() ) exit( 1 );

    if(options[HELP] || argc == 0) {
        int columns = getenv("COLUMNS")? atoi(getenv("COLUMNS")) : 80;
        option::printUsage(std::cout, usage, columns);
        exit( 1 );
    }

    bool verbose, quiet, extract, slow_dxt1;
    options[VERBOSE]   ? verbose = true   : verbose = false;
    options[QUIET]     ? quiet = true     : quiet = false;
    options[EXTRACT]   ? extract = true   : extract = false;
    options[SLOW_DXT1] ? slow_dxt1 = true : slow_dxt1 = false;

    vector<string> sourceFiles;
    for (option::Option* opt = options[SOURCES]; opt; opt = opt->next())
        sourceFiles.push_back(opt->arg);

    int width = 8, length = 8, cnum = 32, cnet = 4, stride = 1;
    float cpet = 1.0f/255.0f;
    string outputPrefix, inputFile, tilemapFile, decalFile;

    for (int i = 0; i < parse.optionsCount(); ++i) {
        option::Option& opt = buffer[i];
        switch(opt.index()) {
        case WIDTH:
            width = atoi(opt.arg);
            break;
        case LENGTH:
            length = atoi(opt.arg);
            break;
        case CNUM:
            cnum = atoi(opt.arg);
            break;
        case CNET:
            cnet = atoi(opt.arg);
            break;
        case CPET:
            cpet = atof(opt.arg);
            break;
        case STRIDE:
            stride = atoi(opt.arg);
            break;
        case INPUT:
            inputFile = opt.arg;
            break;
        case OUTPUT:
            outputPrefix = opt.arg;
            break;
        case DECALS:
            decalFile = opt.arg;
            break;
        }
    }

    delete[] options;
    delete[] buffer;
    
    SMT smt;
    smt.verbose = verbose;
    smt.quiet = quiet;
    smt.slow_dxt1 = slow_dxt1;
    smt.cnum = cnum;
    smt.cpet = cpet;
    smt.cnet = cnet;
    smt.stride = stride;
    smt.setSize( width, length );
    smt.setDecalFile( decalFile );

    if( sourceFiles.size() > 0) {
        vector< string >::iterator it;
        for(it = sourceFiles.begin(); it != sourceFiles.end(); it++ )
            smt.addTileSource( *it );
    }

    if( !outputPrefix.empty() ) {
        smt.setPrefix( outputPrefix );
        smt.save();
    }

    smt.setTilemap( tilemapFile );

    if( !inputFile.empty() ) {
        smt.load( inputFile );
        inputFile.erase( inputFile.size() - 4 );
        smt.setPrefix( inputFile );
        if( extract ) smt.decompile();
    }

    exit(0);
}
