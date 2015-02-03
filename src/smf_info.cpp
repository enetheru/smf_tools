#include "elog/elog.h"
#include "optionparser/optionparser.h"

#include "option_args.h"
#include "smf.h"

enum optionsIndex
{
    UNKNOWN,
    HELP
};

const option::Descriptor usage[] = {
    { UNKNOWN, 0, "", "", Arg::None,
        "USAGE: smf_info <smf file> \n"
        "  eg. 'smf_info myfile.smf'\n"},
    { HELP, 0, "h", "help", Arg::None,
        "  -h,  \t--help  \tPrint usage and exit." },
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

    // unknown options
    for( option::Option* opt = options[ UNKNOWN ]; opt; opt = opt->next() ){
        LOG(INFO) << "Unknown option: " << std::string( opt->name,opt->namelen );
    }

    if( options[ HELP ] || parse.nonOptionsCount() == 0 ) {
        int columns = getenv( "COLUMNS" ) ? atoi( getenv( "COLUMNS" ) ) : 80;
        option::printUsage( std::cout, usage, columns );
        exit( 1 );
    }

    if( parse.error() ) exit( 1 );

    // all non options are treated as smf's options
    SMF *smf;
    int retVal = 0;
    for( int i = 0; i < parse.nonOptionsCount(); ++i ){
        smf = NULL;
        if(! ( smf = SMF::open( parse.nonOption(0)) ) ){
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
