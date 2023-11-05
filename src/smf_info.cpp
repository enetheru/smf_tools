#include <spdlog/spdlog.h>

#include "option_args.h"
#include "smflib/smf.h"

enum optionsIndex {
    UNKNOWN,
    HELP,
    VERBOSE,
    QUIET,
    VERSION
};

const option::Descriptor usage_short[] = {
        { UNKNOWN, 0, "", "", Arg::None,
          "USAGE: smf_info <smf file> \n"
          "  eg. 'smf_info myfile.smt'\n"},
        { 0, 0, nullptr, nullptr, nullptr, nullptr }
};

const option::Descriptor usage[] = {
        {UNKNOWN, 0, "",      "",        Arg::None, ""},
        {HELP,    0, "h",     "help",    Arg::None, "  -h,  \t--help  \tPrint usage and exit."},
        {VERBOSE, 0, "v",     "verbose", Arg::None, "  -v,  \t--verbose  \tMOAR output."},
        {QUIET,   0, "q",     "quiet",   Arg::None, "  -q,  \t--quiet  \tSupress Output."},
        {VERSION, 0, "",      "version", Arg::None, "  -V,  \t--version  \tDisplay Version Information."},
        {0,       0, nullptr, nullptr,   nullptr,   nullptr}
};

static void shutdown( int code ){
    OIIO::shutdown();
    exit( code );
}

int main( int argc, char **argv )
{
    spdlog::set_pattern("[%l] %s:%#:%! | %v");
    spdlog::set_level( spdlog::level::warn );

    // Option parsing
    // ==============
    argc -= (argc > 0); argv += (argc > 0);
    option::Stats stats( usage, argc, argv );
    auto* options = new option::Option[ stats.options_max ];
    auto* buffer = new option::Option[ stats.buffer_max ];
    option::Parser parse( usage, argc, argv, options, buffer );
    int term_columns = getenv("COLUMNS" ) ? atoi(getenv("COLUMNS" ) ) : 80;

    // No arguments
    if( argc == 0 ){
        option::printUsage(std::cout, usage_short, term_columns, 60, 80 );
        shutdown( 1 );
    }

    // unknown options
    if( options[ UNKNOWN ] ){
        for( option::Option* opt = options[ UNKNOWN ]; opt; opt = opt->next() ){
            SPDLOG_ERROR( "Unknown option: {}", std::string( opt->name,opt->namelen ) );
        }
        option::printUsage(std::cout, usage_short, term_columns, 60, 80 );
        option::printUsage(std::cout, usage, term_columns, 60, 80 );
        shutdown( 1 );
    }

    // Version Message
    if( options[ VERSION ] ) {
        fmt::println("Version Information Goes here"); //FIXME add version information
        shutdown( 0 );
    }

    // Help Message
    if( options[ HELP ] ) {
        option::printUsage(std::cout, usage, term_columns, 60, 80);
        shutdown( 0 );
    }

    // setup logging level.
    if( options[ VERBOSE ] )
        spdlog::set_level(spdlog::level::info);
    if( options[ QUIET ] )
        spdlog::set_level(spdlog::level::off);

    if( parse.error() ) {
        SPDLOG_ERROR("Options Parsing Error");
        shutdown(1);
    }

    // End of generic parsing options like quiet, verbose etc.

    // all non-options are treated as smf's options
    int retVal = 0;
    for( int i = 0; i < parse.nonOptionsCount(); ++i ){
        std::unique_ptr<SMF> smf( SMF::open( parse.nonOption( i ) ) );
        if(! smf ){
            SPDLOG_ERROR( "cannot open smf file" );
            retVal = 1;
        } else {
            fmt::println( "{}", smf->json().dump(4) );
        }
    }

    delete [] options;
    delete [] buffer;
    OIIO::shutdown();
    return retVal;
}
