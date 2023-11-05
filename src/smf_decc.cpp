#include <fstream>

#include <OpenImageIO/imagebuf.h>

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
                "USAGE: smf_decc <smf file>\n"
                "  eg. 'smf_decc myfile.smf'\n"},
        { 0, 0, nullptr, nullptr, nullptr, nullptr }
};

const option::Descriptor usage[] = {
        {UNKNOWN, 0, "", "", Arg::None, ""},
        {HELP, 0, "h", "help", Arg::None, "  -h,  \t--help  \tPrint usage and exit."},
        {VERBOSE, 0, "v", "verbose", Arg::None, "  -v,  \t--verbose  \tMOAR output."},
        {QUIET, 0, "q", "quiet", Arg::None, "  -q,  \t--quiet  \tSupress Output."},
        {VERSION, 0, "", "version", Arg::None, "  -V,  \t--version  \tDisplay Version Information."},
//TODO add output prefix as an option, rather than solely relying on the input filename
        {0, 0, nullptr, nullptr, nullptr, nullptr}
};

static void shutdown( int code ){
    OIIO::shutdown();
    exit( code );
}

int
main( int argc, char **argv )
{
    spdlog::set_pattern("[%l] %s:%#:%! | %v");
    spdlog::set_level( spdlog::level::warn );

    // Option parsing
    // ==============
    argc -= (argc > 0); argv += (argc > 0);
    bool arg_fail = false;
    option::Stats stats( usage, argc, argv );
    std::vector<option::Option> options(stats.options_max );
    std::vector<option::Option> buffer(stats.options_max );
    option::Parser parse( usage, argc, argv, options.data(), buffer.data() );
    int term_columns = getenv("COLUMNS" ) ? atoi(getenv("COLUMNS" ) ) : 80;

    // No arguments
    if( argc == 0 ){
        option::printUsage(std::cout, usage_short, term_columns, 60, 80 );
        shutdown( 1 );
    }

    // unknown options
    for( option::Option* opt = options[ UNKNOWN ]; opt; opt = opt->next() ){
        SPDLOG_ERROR( "Unknown option: {}", std::string( opt->name,opt->namelen ) );
        arg_fail = true;
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

    if( parse.error() || arg_fail ) {
        SPDLOG_ERROR("Options Parsing Error");
        shutdown(1);
    }

    // End of generic parsing options like quiet, verbose etc.

    // non options
    if( parse.nonOptionsCount() == 0 ){
        SPDLOG_WARN( "Missing input filename" );
        arg_fail = true;
    }

    for( int i = 1; i < parse.nonOptionsCount(); ++i ){
        SPDLOG_WARN( "Superfluous Argument: {}", parse.nonOption( i ) );
        arg_fail = true;
    }

    if( arg_fail || parse.error() ){
        SPDLOG_ERROR( "Options parsing" );
        shutdown( 1 );
    }
    // end of options parsing

    SMF *smf;
    if(! ( smf = SMF::open( parse.nonOption(0)) ) ){
        SPDLOG_ERROR( "cannot open {}", parse.nonOption(0) );
        shutdown( 1 );
    }

    std::fstream file;
    OIIO::ImageBuf *buf; //FIXME is buf really needed here?
    TileMap *tileMap;

    SPDLOG_INFO( "Extracting Header Info" );
    file.open( "out_Header_Info.txt", std::ios::out );
    file << smf->json().dump(4);
    file.close();

    SPDLOG_INFO( "Extracting height image" );
    buf = smf->getHeight();
    buf->write( "out_height.tif", OIIO::TypeUnknown, "tif" );


    SPDLOG_INFO( "Extracting type image" );
    buf = smf->getType();
    buf->write("out_type.tif", OIIO::TypeUnknown, "tif");

    SPDLOG_INFO( "Extracting map image" );
    tileMap = smf->getMap();
    file.open("out_tilemap.csv", std::ios::out );
    file << tileMap->toCSV();
    file.close();

    SPDLOG_INFO( "Extracting mini image" );
    buf = smf->getMini();
    buf->write("out_mini.tif", OIIO::TypeUnknown, "tif");

    SPDLOG_INFO( "Extracting metal image" );
    buf = smf->getMetal();
    buf->write("out_metal.tif", OIIO::TypeUnknown, "tif");

    SPDLOG_INFO( "Extracting featureList" );
    file.open( "out_featuretypes.txt", std::ios::out );
    file << smf->getFeatureTypes();
    file.close();

    SPDLOG_INFO( "Extracting features" );
    file.open( "out_features.csv", std::ios::out );
    file << smf->getFeatures();
    file.close();

    buf = smf->getGrass();
    if( buf ){
        SPDLOG_INFO( "Extracting grass image" );
        buf->write("out_grass.tif", OIIO::TypeUnknown, "tif");
    }
    OIIO::shutdown();
    return 0;
}
