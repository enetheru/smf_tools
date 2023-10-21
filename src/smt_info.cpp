#include <spdlog/spdlog.h>

#include "option_args.h"
#include "smt.h"

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


int main( int argc, char **argv )
{
    // Option parsing
    // ==============
    argc -= (argc > 0); argv += (argc > 0);
    option::Stats stats( usage, argc, argv );
    auto* options = new option::Option[ stats.options_max ];
    auto* buffer = new option::Option[ stats.buffer_max ];
    option::Parser parse( usage, argc, argv, options, buffer );

    if( options[ HELP ] || argc == 0 ) {
        int columns = getenv( "COLUMNS" ) ? atoi( getenv( "COLUMNS" ) ) : 80;
        option::printUsage( std::cout, usage, columns );
        exit( 1 );
    }

    if( options[ QUIET ] ) spdlog::set_level(spdlog::level::off);


    // unknown options
    for( option::Option* opt = options[ UNKNOWN ]; opt; opt = opt->next() ){
        spdlog::warn( "Unknown option: {}", std::string( opt->name,opt->namelen ) );
    }

    // non options
    for( int i = 1; i < parse.nonOptionsCount(); ++i ){
        spdlog::warn( "Unknown Option: {}", parse.nonOption( i ) );
    }

    if( parse.error() ) exit( 1 );

    SMT *smt;
    if(! ( smt = SMT::open( parse.nonOption(0)) ) ){
        spdlog::critical( "cannot open smt file" );
        exit(1);
    }

    spdlog::info( smt->info() );

    return 0;
}
