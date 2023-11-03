#include <fstream>

#include <spdlog/spdlog.h>

#include "option_args.h"
#include "smflib/smt.h"

enum optionsIndex
{
    UNKNOWN,
    // General Options
    HELP, VERBOSE, QUIET,
    // File Operations
    IFILE, OVERWRITE
};

const option::Descriptor usage[] = {
    { UNKNOWN, 0, "", "", Arg::None,
        "USAGE: smt_cc [options] [source files] \n"
        "  eg. 'smt_cc -v -f mysmt.smt *.jpg'\n"
        "\nGENERAL OPTIONS:" },
    { HELP, 0, "h", "help", Arg::None,
        "  -h,  \t--help  \tPrint usage and exit." },
    { VERBOSE, 0, "v", "verbose", Arg::None,
        "  -v,  \t--verbose  \tPrint extra information." },
    { QUIET, 0, "q", "quiet", Arg::None,
        "  -q,  \t--quiet  \tSupress output." },
    { OVERWRITE, 0, "f", "force", Arg::None,
        "  -f,  \t--force  \toverwrite existing files." },
    { IFILE, 0, "o", "file", Arg::Required,
        "  -f,  \t--file=filename.smt  \tfile to operate on, will be created "
            "if it doesnt exist." },
    { UNKNOWN, 0, "", "", Arg::None,
        "\nEXAMPLES:\n"
        "  smt_cc \n"
    },
    { 0, 0, nullptr, nullptr, nullptr, nullptr }
};

static void shutdown( int code ){
    OIIO::shutdown();
    exit( code );
}

int
main( int argc, char **argv )
{
    spdlog::set_pattern("[%l] %s:%#:%! | %v");    // Option parsing
    // ==============
    bool fail = false;
    argc -= (argc > 0); argv += (argc > 0);
    option::Stats stats( usage, argc, argv );
    auto* options = new option::Option[ stats.options_max ];
    auto* buffer = new option::Option[ stats.buffer_max ];
    option::Parser parse( usage, argc, argv, options, buffer );

    if( options[ HELP ] || argc == 0 ) {
        int columns = getenv( "COLUMNS" ) ? atoi( getenv( "COLUMNS" ) ) : 80;
        option::printUsage( std::cout, usage, columns );
        shutdown( options[ HELP ] ? 0 : 1 );
    }

    // setup logging level.
    spdlog::set_level(spdlog::level::warn);
    if( options[ VERBOSE ] )
        spdlog::set_level(spdlog::level::info);
    if( options[ QUIET ] )
        spdlog::set_level(spdlog::level::off);

    // unknown options
    for( option::Option* opt = options[ UNKNOWN ]; opt; opt = opt->next() ){
        SPDLOG_WARN( "Unknown option: {}", std::string( opt->name,opt->namelen ) );
        fail = true;
    }

    // non options
    if( parse.nonOptionsCount() == 0 ){
        SPDLOG_ERROR( "no file given" );
        shutdown( 1 );
    }
    else if( parse.nonOptionsCount() > 1 ){
        SPDLOG_ERROR( "too many arguments" );
        shutdown( 1 );
   }

    if( fail || parse.error() ){
        SPDLOG_ERROR( "Options parsing" );
        shutdown( 1 );
    }

// test file properties
    SMT *smt;
    if(! (smt = SMT::open( parse.nonOption( 0 ) )) ){
        SPDLOG_ERROR( "\nunable to open file" );
        shutdown ( 1 );
    }
    SPDLOG_INFO( smt->info() );

    std::fstream inFile( parse.nonOption( 0 ), std::ios::in );
    inFile.seekg( 0, std::ios::end );
    uint32_t inSize = inFile.tellg();
    inFile.close();

    SPDLOG_INFO( "{} bytes", inSize );
    uint32_t tiles = (inSize - 32) / smt->getTileBytes();
    SPDLOG_INFO( "{} tiles", tiles);
    inFile.open( parse.nonOption( 0 ), std::ios::out | std::ios::in );
    inFile.seekp(20);
    inFile.write( (char *)&tiles, 4 );
    inFile.close();

// fix file
// output file

    delete[] options;
    delete[] buffer;
    OIIO::shutdown();
    return 0;
}
