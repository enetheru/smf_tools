
#include <fstream>

#include "optionparser.h"
#include "smt.h"

void valxval( string s, int &x, int &y);
vector<unsigned int> expandString( const char *s );

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

enum optionsIndex
{
    UNKNOWN,
    // General Options
    HELP,
    VERBOSE,
    QUIET,
    // Construction
    MAPSIZE,
    TILESIZE,
    SOURCE,
    STRIDE,
    DECALS,
    OUTPUT,
    // Compression
    SLOW_DXT1,
    CNUM, CPET, CNET,
    // Deconstruction
    INPUT,
    EXTRACT,
    COLLATE,
    RECONSTRUCT,
    IMAGESIZE,
    APPEND
};

const option::Descriptor usage[] = {
    { UNKNOWN, 0, "", "", Arg::None,
        "USAGE: tileconv [options]\n"
        "\nGENERAL OPTIONS:" },
    { HELP, 0, "h", "help", Arg::None,
        "  -h,  \t--help  \tPrint usage and exit." },
    { VERBOSE, 0, "v", "verbose", Arg::None,
        "  -v,  \t--verbose  \tPrint extra information." },
    { QUIET, 0, "q", "quiet", Arg::None,
        "  -q,  \t--quiet  \tSupress output." },

    { UNKNOWN, 0, "", "", Arg::None,
        "\nCONSTRUCTION OPTIONS:" },
    { MAPSIZE, 0, "", "mapsize", Arg::Required,
        "\t--mapsize  \tWidth and length of map, in spring map units eg. '--mapsize=4x4', must be multiples of two." },
    { TILESIZE, 0, "", "tilesize", Arg::Numeric,
        "\t--tilesize  \tXY resolution of tiles to save, eg. '--tilesize=32'." },
    { SOURCE, 0, "", "source", Arg::Required,
        "  -s,  \t--source  \tSource files to use for tiles." },
    { STRIDE, 0, "", "stride", Arg::Numeric,
        "\t--stride  \tNumber of source tiles horizontally before wrapping." },
    { DECALS, 0, "", "decals", Arg::Required,
        "\t--decals  \tFile to parse when pasting decals." },
    { OUTPUT, 0, "o", "output", Arg::Required,
        "  -o,  \t--output  \tOutput filename used when saving." },
    { APPEND, 0, "a", "append", Arg::Required,
        "  -a,  \t--append  \tTreat images as tiles, scale and save individually." },

    { UNKNOWN, 0, "", "", Arg::None,
        "\nCOMPRESSION OPTIONS:" },
    { SLOW_DXT1, 0, "", "slow_dxt1", Arg::None,
        "\t--slow_dxt1  \tUse slower but better analytics when compressing DXT1 textures" },
    { CNUM, 0, "", "cnum", Arg::Numeric,
        "\t--cnum  \tNumber of tiles to compare; n=-1, no comparison; n=0, hashtable exact comparison; n > 0, numeric comparison between n tiles" },
    { CPET, 0, "", "cpet", Arg::Numeric,
        "\t--cpet  \tPixel error threshold. 0.0f-1.0f" },
    { CNET, 0, "", "cnet", Arg::Numeric,
        "\t--cnet  \tErrors threshold 0-1024." },

    { UNKNOWN, 0, "", "", Arg::None,
        "\nDECONSTRUCTION OPTIONS:" },
    { INPUT, 0, "i", "input", Arg::Required,
        "  -i,  \t--input  \tSMT filename to load." },
    { EXTRACT, 0, "e", "extract", Arg::Optional,
        "  -e,  \t--extract  \tExtract images from loaded SMT." },
    { COLLATE, 0, "", "collate", Arg::None,
        "  \t--collate  \tCollate the extracted tiles." },
    { RECONSTRUCT, 0, "", "reconstruct", Arg::Required,
        "  \t--reconstruct  \tReconstruct the extracted tiles using a tilemap." },
    { IMAGESIZE, 0, "", "imagesize", Arg::Required,
        "  \t--imagesize  \tScale the resultant extraction to this size, eg. '--imagesize=1024x768'." },

    { UNKNOWN, 0, "", "", Arg::None,
        "\nEXAMPLES:\n"
        "  tileconv --sources diffuse.jpg -o filename\n"
        "  tileconv -i test.smt -e --reconstruct tilemap.exr"
        "  tileconv --sources=image_{00..07}_{00..07}.jpg --stride 8 --mapsize=16x16 -o mymap.smt"
        "  tileconv --stride 8 --mapsize=16x16 -o mymap.smt -- image_*.jpg"
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

    if( parse.error() ) exit( 1 );

    if( options[ HELP ] || argc == 0 ) {
        int columns = getenv( "COLUMNS" ) ? atoi( getenv( "COLUMNS" ) ) : 80;
        option::printUsage( std::cout, usage, columns );
        exit( 1 );
    }

    SMT smt;

    options[ VERBOSE   ] ? smt.verbose   = true : smt.verbose   = false;
    options[ QUIET     ] ? smt.quiet     = true : smt.quiet     = false;
    options[ SLOW_DXT1 ] ? smt.slow_dxt1 = true : smt.slow_dxt1 = false;
    
    int x,y;
    vector<unsigned int>tileNums;
    for( int i = 0; i < parse.optionsCount(); ++i ) {
        option::Option& opt = buffer[ i ];
        switch( opt.index() ) {

        case MAPSIZE:
            valxval( opt.arg, x, y );
            smt.setSize( x, y );
            break;

        case TILESIZE:
            smt.setTileRes( stoi( opt.arg ) );
            break;

        case STRIDE:
            smt.stride = stoi( opt.arg );
            break;

        case DECALS:
            smt.setDecalFN( opt.arg );
            break;

        case OUTPUT:
            smt.setSaveFN( opt.arg );
            break;

        case CNUM:
            smt.cnet = stoi( opt.arg );
            break;

        case CNET:
            smt.cnet = stoi( opt.arg );
            break;

        case CPET:
            smt.cpet = stof( opt.arg );
            break;

        case INPUT:
            smt.setLoadFN( opt.arg );
            break;

        case EXTRACT:
            if( opt.arg ) tileNums = expandString( opt.arg );
            break;

        case RECONSTRUCT:
            smt.setTilemapFN( opt.arg );
            break;

        case IMAGESIZE:
            valxval( opt.arg, x, y );
            //FIXME somehow scale the result of extract to this
            break;
        }
    }

    // Source Files
    for( option::Option* opt = options[ SOURCE ]; opt; opt = opt->next() )
        smt.addTileSource( opt->arg );

    // FIXME somehow error when nonOpions are specified before actual options.
    for( int i = 0; i < parse.nonOptionsCount(); ++i )
        smt.addTileSource( parse.nonOption( i ) );

    if(! options[ STRIDE ] ){
        if( options[ SOURCE ] ){
           smt.stride = ceil( sqrt( smt.getSourceFiles().size() ) );
        }
    }
    // FIXME - auto detect stride

///    if(! options[ MAPSIZE ] )
    // FIXME - auto detect map size

    if( options[ OUTPUT ] )
    {
        if( options[ SOURCE ] )
        {
            if( options[ APPEND ] )
                smt.save2();
            else smt.save();
        }
    }

    if( options[ INPUT ] )
    {
        smt.load();

        // decompiling
        if( options[ RECONSTRUCT ] )
        {
            cout << "reconstruct" << endl;
            smt.decompile();
        }
        else if( options[ COLLATE ] )
        {
            cout << "Collate" << endl;
            smt.decompile();
        }
        else if( options[ EXTRACT ] )
        {
            //TODO
            cout << "individuals not yet implemented" << endl;
        }

        if( options[ APPEND ] ){
            ///TODO
        }
    }

    delete[] options;
    delete[] buffer;
    exit(0);
}

void
valxval( string s, int &x, int &y)
{
    unsigned int d;
    d = s.find_first_of( 'x', 0 );

    if(d) x = stoi( s.substr( 0, d ) );
    else x = 0;

    if(d == s.size()-1 ) y = 0;
    else y = stoi( s.substr( d + 1, string::npos) );
}

vector<unsigned int>
expandString( const char *s )
{
    vector<unsigned int> result;

    int start;
    bool sequence = false;
    const char *begin;
    do {
        begin = s;

        while( *s != ',' && *s != '-' && *s ) s++;
        if( begin == s) continue;

        if( sequence )
        {
            for( int i = start; i < stoi( string( begin, s ) ); ++i )
                result.push_back( i );
        }

        if( *(s) == '-' )
        {
            sequence = true;
            start = stoi( string( begin, s ) );
        } else {
            sequence = false;
            result.push_back( stoi( string( begin, s ) ) );
        }
    }
    while( *s++ != '\0' );

    return result;

}
