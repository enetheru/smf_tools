#include <elog.h>

#include "option_args.h"
#include "smf.h"

enum optionsIndex
{
    UNKNOWN,
    HELP,
    QUIET
};

const option::Descriptor usage[] = {
    { UNKNOWN, 0, "", "", Arg::None,
        "USAGE: smf_info <smf file> \n"
        "  eg. 'smf_info myfile.smf'\n"},
    { HELP, 0, "h", "help", Arg::None,
        "  -h,  \t--help  \tPrint usage and exit." },
    { QUIET, 0, "q", "quiet", Arg::None,
        "  -q,  \t--quiet  \tSupress output, except warnings and errors" },
    { 0, 0, 0, 0, 0, 0 }
};


int main( int argc, char **argv )
{
    // Option parsing
    // ==============
    argc -= (argc > 0); argv += (argc > 0);
    option::Stats stats( usage, argc, argv );
    option::Option* options = new option::Option[ stats.options_max ];
    option::Option* buffer = new option::Option[ stats.buffer_max ];
    option::Parser parse( usage, argc, argv, options, buffer );

    if( options[ HELP ] || parse.nonOptionsCount() == 0 ) {
        int columns = getenv( "COLUMNS" ) ? atoi( getenv( "COLUMNS" ) ) : 80;
        option::printUsage( std::cout, usage, columns );
        if( options[ HELP ] ) exit( 0 );
        else exit( 1 );
    }

    if( options[ QUIET ] )LOG::SetDefaultLoggerLevel( LOG::CHECK );

    // unknown options
    for( option::Option* opt = options[ UNKNOWN ]; opt; opt = opt->next() ){
        LOG( ERROR ) << "Unknown option: " << std::string( opt->name,opt->namelen );
    }

    if( parse.error() ) exit( 1 );

    // all non options are treated as smf's options
    SMF *smf;
    int retVal = 0;
    for( int i = 0; i < parse.nonOptionsCount(); ++i ){
        smf = SMF::open( parse.nonOption( i ) );
        if(! smf ){
            LOG( ERROR ) << "cannot open smf file";
            retVal = 1;
        }
        else {
            LOG(INFO) << "\n" << smf->info();
            smf->good();
            delete smf;
        }
    }

    delete [] options;
    delete [] buffer;
    return retVal;
}
