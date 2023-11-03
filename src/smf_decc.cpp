#include <fstream>

#include <OpenImageIO/imagebuf.h>

#include <spdlog/spdlog.h>

#include "option_args.h"
#include "smf.h"

enum optionsIndex
{
    UNKNOWN,
    VERBOSE,
    QUIET,
    HELP
};

const option::Descriptor usage[] = {
    { UNKNOWN, 0, "", "", Arg::None,
        "USAGE: smf_decc <smf file>\n"
        "  eg. 'smf_decc myfile.smf'\n"},
    { HELP, 0, "h", "help", Arg::None,
        "  -h,  \t--help  \tPrint usage and exit." },
    { VERBOSE, 0, "v", "verbose", Arg::None,
        "  -v,  \t--verbose  \tMOAR output." },
    { QUIET, 0, "q", "quiet", Arg::None,
        "  -q,  \t--quiet  \tSupress Output." },
//TODO add output prefix as an option, rather than solely relying on the input filename
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

    // Help Message
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
        SPDLOG_WARN( "Missing input filename" );
        fail = true;
    }

    for( int i = 1; i < parse.nonOptionsCount(); ++i ){
        SPDLOG_WARN( "Superfluous Argument: {}", parse.nonOption( i ) );
        fail = true;
    }

    if( fail || parse.error() ){
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
    file << smf->info();
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
