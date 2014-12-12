#include "smf.h"
#include "util.h"
#include "tilemap.h"

#include "elog/elog.h"
#include "optionparser/optionparser.h"

#include <OpenImageIO/imagebuf.h>
#include <fstream>
#include <string>

// Argument tests //
////////////////////
struct Arg: public option::Arg
{
    static void printError(const char* msg1, const option::Option& opt,
            const char* msg2)
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

        if (msg) printError("Option '", option,
                "' requires a numeric argument\n");
        return option::ARG_ILLEGAL;
    }
};

enum optionsIndex
{
    UNKNOWN,
    HELP
};

const option::Descriptor usage[] = {
    { UNKNOWN, 0, "", "", Arg::None,
        "USAGE: smf_decc <smf file>\n"
        "  eg. 'smf_decc myfile.smf'\n"},
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

    // setup logging level.
    LOG::SetDefaultLoggerLevel( LOG::WARN );
    if( options[ VERBOSE ] )
        LOG::SetDefaultLoggerLevel( LOG::INFO );
    if( options[ QUIET ] )
        LOG::SetDefaultLoggerLevel( LOG::CHECK );

    // unknown options
    for( option::Option* opt = options[ UNKNOWN ]; opt; opt = opt->next() ){
        LOG(INFO) << "Unknown option: " << std::string( opt->name,opt->namelen );
    }

    // non options
    for( int i = 1; i < parse.nonOptionsCount(); ++i ){
        LOG(INFO) << "Unknown Option " << parse.nonOption( i );
    }

    if( parse.error() ) exit( 1 );

    if( options[ HELP ] || argc == 0 ) {
        int columns = getenv( "COLUMNS" ) ? atoi( getenv( "COLUMNS" ) ) : 80;
        option::printUsage( std::cout, usage, columns );
        exit( 1 );
    }

    SMF *smf = NULL;
    if(! ( smf = SMF::open( parse.nonOption(0)) ) ){
        LOG(FATAL) << "cannot open smf file";
    }

    std::fstream file;
    OpenImageIO::ImageBuf *buf = NULL;
    TileMap *tileMap = NULL;

    LOG(INFO) << "INFO: Extracting Header Info";
    file.open( "out_Header_Info.txt", std::ios::out );
    file << smf->info();
    file.close();


    LOG(INFO) << "INFO: Extracting height image";
    buf = smf->getHeight();
    buf->write("out_height.tif", "tif" );

    LOG(INFO) << "INFO: Extracting type image";
    buf = smf->getType();
    buf->write("out_type.tif", "tif");

    LOG(INFO) << "INFO: Extracting map image";
    tileMap = smf->getMap();
    file.open("out_tilemap.csv", std::ios::out );
    file << tileMap->toCSV();
    file.close();

    LOG(INFO) << "INFO: Extracting mini image";
    buf = smf->getMini();
    buf->write("out_mini.tif", "tif");

    LOG(INFO) << "INFO: Extracting metal image";
    buf = smf->getMetal();
    buf->write("out_metal.tif", "tif");

    LOG(INFO) << "INFO: Extracting featureList";
    file.open( "out_featuretypes.txt", std::ios::out );
    file << smf->getFeatureTypes();
    file.close();

    LOG(INFO) << "INFO: Extracting features";
    file.open( "out_features.csv", std::ios::out );
    file << smf->getFeatures();
    file.close();

    buf = smf->getGrass();
    if( buf ){
        LOG(INFO) << "INFO: Extracting grass image";
        buf->write("out_grass.tif", "tif");
    }

    return 0;
}
