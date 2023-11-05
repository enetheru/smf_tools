#include <spdlog/spdlog.h>

#include "option_args.h"
#include "smflib/smt.h"

enum optionsIndex
{
    UNKNOWN,
    HELP,
    QUIET
};

const option::Descriptor usage[] = {
    { UNKNOWN, 0, "", "", Arg::None,
        "USAGE: smtinfo <smt file> \n"
        "  eg. 'smtinfo myfile.smt'\n"},
    { HELP, 0, "h", "help", Arg::None,
        "  -h,  \t--help  \tPrint usage and exit." },
    { QUIET, 0, "q", "quiet", Arg::None,
        "  -q,  \t--quiet  \tSupress output, except warnings and errors" },
    { 0, 0, nullptr, nullptr, nullptr, nullptr }
};

static void shutdown( int code ){
    OIIO::shutdown();
    exit( code );
}


int main( int argc, char **argv )
{
    spdlog::set_pattern("[%l] %s:%#:%! | %v");    // Option parsing
    // ==============
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

    if( options[ QUIET ] ) spdlog::set_level(spdlog::level::off);


    // unknown options
    for( option::Option* opt = options[ UNKNOWN ]; opt; opt = opt->next() ){
        SPDLOG_WARN( "Unknown option: {}", std::string( opt->name,opt->namelen ) );
    }

    // non options
    for( int i = 1; i < parse.nonOptionsCount(); ++i ){
        SPDLOG_WARN( "Unknown Option: {}", parse.nonOption( i ) );
    }

    if( parse.error() ) shutdown( 1 );

    SMT *smt;
    if(! ( smt = SMT::open( parse.nonOption(0)) ) ){
        SPDLOG_CRITICAL( "cannot open smt file" );
        shutdown(1);
    }

    SPDLOG_INFO( smt->json().dump(4) );
    OIIO::shutdown();
    return 0;
}
