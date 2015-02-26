#include <fstream>

#include "optionparser/optionparser.h"
#include "elog/elog.h"

#include "option_args.h"
#include "smt.h"

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
    { 0, 0, 0, 0, 0, 0 }
};

int
main( int argc, char **argv )
{
    // Option parsing
    // ==============
    bool fail = false;
    argc -= (argc > 0); argv += (argc > 0);
    option::Stats stats( usage, argc, argv );
    option::Option* options = new option::Option[ stats.options_max ];
    option::Option* buffer = new option::Option[ stats.buffer_max ];
    option::Parser parse( usage, argc, argv, options, buffer );

    if( options[ HELP ] || argc == 0 ) {
        int columns = getenv( "COLUMNS" ) ? atoi( getenv( "COLUMNS" ) ) : 80;
        option::printUsage( std::cout, usage, columns );
        exit( 1 );
    }

    // setup logging level.
    LOG::SetDefaultLoggerLevel( LOG::WARN );
    if( options[ VERBOSE ] )
        LOG::SetDefaultLoggerLevel( LOG::INFO );
    if( options[ QUIET ] )
        LOG::SetDefaultLoggerLevel( LOG::CHECK );

    // unknown options
    for( option::Option* opt = options[ UNKNOWN ]; opt; opt = opt->next() ){
        LOG( WARN ) << "Unknown option: " << std::string( opt->name,opt->namelen );
        fail = true;
    }

    // non options
    if( parse.nonOptionsCount() == 0 ){
        LOG( ERROR ) << "no file given";
        exit( 1 );
    }
    else if( parse.nonOptionsCount() > 1 ){
        LOG( ERROR ) << "too many arguments";
        exit( 1 );
   }

    if( fail || parse.error() ){
        LOG( ERROR ) << "Options parsing";
        exit( 1 );
    }

// test file properties
    SMT *smt = NULL;
    if(! (smt = SMT::open( parse.nonOption( 0 ) )) ){
        LOG( ERROR ) << "\nunable to open file";
        exit ( 1 );
    }
    LOG( INFO ) << smt->info();

    std::fstream inFile( parse.nonOption( 0 ), std::ios::in );
    inFile.seekg( 0, std::ios::end );
    int inSize = inFile.tellg();
    inFile.close();

    LOG( INFO ) << inSize << " bytes";
    int tiles = (inSize - 32) / smt->tileBytes;
    LOG( INFO ) << tiles << " tiles";
    inFile.open( parse.nonOption( 0 ), std::ios::out | std::ios::in );
    inFile.seekp(20);
    inFile.write( (char *)&tiles, 4 );
    inFile.close();

// fix file
// output file

    delete[] options;
    delete[] buffer;

    exit( 0 );
}
