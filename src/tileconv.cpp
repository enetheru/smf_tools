
#include <fstream>

#include "optionparser.h"
#include "smt.h"
#include "smtool.h"
#include "util.h"
#include "tilecache.h"

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>

using namespace std;
OIIO_NAMESPACE_USING;

void valxval( string s, unsigned int &x, unsigned int &y);
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
};

enum optionsIndex
{
    UNKNOWN,
    // General Options
    HELP, VERBOSE, QUIET,
    //Specification
    MAPSIZE,
    TILESIZE,
    IMAGESIZE,
    STRIDE,
    // Creation
    DECALS,
    FILTER,
    OUTPUT,
    // Compression
    SLOW_DXT1,
    CNUM, CPET, CNET,
    // Deconstruction
    SEPARATE,
    COLLATE,
    RECONSTRUCT
};

const option::Descriptor usage[] = {
    { UNKNOWN, 0, "", "", Arg::None,
        "USAGE: tileconv [options] [source files] \n"
        "  eg. 'tileconv -v -o mysmt.smt *.jpg'\n"
        "\nGENERAL OPTIONS:" },
    { HELP, 0, "h", "help", Arg::None,
        "  -h,  \t--help  \tPrint usage and exit." },
    { VERBOSE, 0, "v", "verbose", Arg::None,
        "  -v,  \t--verbose  \tPrint extra information." },
    { QUIET, 0, "q", "quiet", Arg::None,
        "  -q,  \t--quiet  \tSupress output." },

    { UNKNOWN, 0, "", "", Arg::None,
        "\nSPECIFICATIONS:" },
    { MAPSIZE, 0, "", "mapsize", Arg::Required,
        "\t--mapsize=XxY  \tWidth and length of map, in spring map units eg. '--mapsize=4x4', must be multiples of two." },
    { TILESIZE, 0, "", "tilesize", Arg::Numeric,
        "\t--tilesize=X  \tXY resolution of tiles to save, eg. '--tilesize=32'." },
    { IMAGESIZE, 0, "", "imagesize", Arg::Required,
        "\t--imagesize=XxY  \tScale the resultant extraction to this size, eg. '--imagesize=1024x768'." },
    { STRIDE, 0, "", "stride", Arg::Numeric,
        "\t--stride=N  \tNumber of source tiles horizontally before wrapping." },
    
    { UNKNOWN, 0, "", "", Arg::None,
        "\nCREATION:" },
//    { DECALS, 0, "", "decals", Arg::Required,
//        "\t--decals=filename.csv  \tFile to parse when pasting decals." },
    { FILTER, 0, "", "filter", Arg::Required,
        "\t--filter=[1,2,n,1-n]  \tAppend only these tiles" },
    { OUTPUT, 0, "o", "output", Arg::Required,
        "  -o,  \t--output=filename.smt  \tfilename to output." },

    { UNKNOWN, 0, "", "", Arg::None,
        "\nCOMPRESSION OPTIONS:" },
    { SLOW_DXT1, 0, "", "slow_dxt1", Arg::None,
        "\t--slow_dxt1  \tUse slower but better analytics when compressing DXT1 textures" },
    { CNUM, 0, "", "cnum", Arg::Numeric,
        "\t--cnum=[-1,0,N]  \tNumber of tiles to compare; n=-1, no comparison; n=0, hashtable exact comparison; n > 0, numeric comparison between n tiles" },
    { CPET, 0, "", "cpet", Arg::Numeric,
        "\t--cpet  \tPixel error threshold. 0.0f-1.0f" },
    { CNET, 0, "", "cnet=[0.0-1.0]", Arg::Numeric,
        "\t--cnet=[0-N]  \tErrors threshold 0-1024." },

    { UNKNOWN, 0, "", "", Arg::None,
        "\nDECONSTRUCTION OPTIONS:" },
    { SEPARATE, 0, "", "separate", Arg::None,
        "  \t--separate  \tSplit the source files into individual images." },
    { COLLATE, 0, "", "collate", Arg::None,
        "  \t--collate  \tCollate the extracted tiles." },
    { RECONSTRUCT, 0, "", "reconstruct", Arg::Required,
        "  \t--reconstruct=tilemap.exr  \tReconstruct the extracted tiles using a tilemap." },

    { UNKNOWN, 0, "", "", Arg::None,
        "\nEXAMPLES:\n"
        "  tileconv \n"
        "  tileconv"
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

    // Setup
    // =====
    //
    bool verbose = false, quiet = false;
    if( options[ VERBOSE   ] ) verbose = true;
    if( options[ QUIET     ] ) quiet = true;

    SMT smt( verbose, quiet );
    SMTool::verbose = verbose;
    SMTool::quiet = quiet;
    TileCache tileCache( verbose, quiet );

    // output creation
    if( options[ OUTPUT ] ){
        if( SMTool::create( options[ OUTPUT ].arg ) ){
            exit(1);
        }
        smt.setFileName( options[ OUTPUT ].arg );
    }

    // Add to TileCache
    for( int i = 0; i < parse.nonOptionsCount(); ++i )
        tileCache.push_back( parse.nonOption( i ) );

    if( options[ VERBOSE ] ){
        cout << "INFO: Tiles in cache: " << tileCache.getNTiles() << endl;
        if( tileCache.getNTiles() == 1 )
            cout << "\tAssuming large map provided" << endl;
    }

    // load up the tilemap
    ImageBuf *tilemap = NULL;
    if( options[ RECONSTRUCT ] ){
        tilemap = SMTool::openTilemap( options[ RECONSTRUCT ].arg );
    }

    // Process Options
    if( options[ SLOW_DXT1 ] ) smt.slow_dxt1 = true;

    //TileSize
    if( options[ TILESIZE ] ){
        tileCache.tileSize = stoi( options[ TILESIZE ].arg );
        smt.setTileRes( tileCache.tileSize );
    }

    //filter list
    vector<unsigned int> filter;
    if( options[ FILTER ] ){
        filter = expandString( options[ FILTER ].arg );
        for( auto i = filter.begin(); i != filter.end(); ++i)
            cout << *i << endl;
    }
    else {
        // have to generate a vector with consecutive numbering;
        filter.resize( tileCache.getNTiles() );
        for(unsigned int i = 0; i < filter.size(); ++i) filter[i] = i;
    }

    // MapSize & Stride
    unsigned int mx = 2, my = 2;
    unsigned int hstride = 0, vstride = 0;
    if( options[ MAPSIZE ] ){
        valxval( options[ MAPSIZE ].arg, mx, my);
        hstride = mx * 512 / tileCache.tileSize;
        vstride = my * 512 / tileCache.tileSize;
    }
    else if( options[ STRIDE ] ){
        hstride = stoi( options[ STRIDE ].arg );
    }

    // Image Size
    unsigned int ix = 0, iy = 0;
    if( options[ IMAGESIZE ] ) valxval( options[ IMAGESIZE ].arg, ix, iy);


    // Setup
    ImageBuf *buf = NULL;
    ImageSpec spec;
    ImageBuf fix;
    ROI iroi( 0, ix, 0, iy, 0, 1, 0, 4 );
    ROI mroi( 0, mx * 512, 0, my * 512, 0, 1, 0, 4 );


    // Process tiles Seperately
    if( options[ SEPARATE ] ){
        for(auto i = filter.begin(); i != filter.end(); ++i ){
            buf = tileCache.getTile( *i );
            if( options[ OUTPUT ] ){
                smt.append( buf );
            }
            else {
                buf->save( "tile_" + to_string( *i ) + ".tif", "tif");
            }
            delete buf;
        }
        exit(0);
    }

    // reconstruct / collate Tiles
    string imagename;
    if( options[ RECONSTRUCT ] ){
        buf = SMTool::reconstruct( tileCache, tilemap );
        imagename = "reconstruct.tif";
    }
    else if( options[ COLLATE ] ){
        buf = SMTool::collate( tileCache, hstride, vstride );
        imagename = "collate.tif";
    }
    if( buf ) fix.copy( *buf );
    else exit(1);

    if(! options[ OUTPUT ] ){
        if( options[ IMAGESIZE ] ){
            ImageBufAlgo::resample( *buf, fix, false, iroi);
        }
        buf->save( imagename, "tif" );
        if( verbose ) cout << "INFO: Image saved as " << imagename << endl;
    }
    else {
        if( options[ MAPSIZE ] ){
            ImageBufAlgo::resample( *buf, fix, false, mroi);
        }
        // TODO PROCESS
        SMTool::imageToSMT(buf, smt);
    }

    delete[] options;
    delete[] buffer;
    exit(0);
}

void
valxval( string s, unsigned int &x, unsigned int &y)
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

        if( sequence ){
            for( int i = start; i < stoi( string( begin, s ) ); ++i )
                result.push_back( i );
        }

        if( *(s) == '-' ){
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
