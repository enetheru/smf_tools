#include <fstream>

#include <OpenImageIO/imagebuf.h>

#include <elog.h>

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
    { 0, 0, nullptr, nullptr, nullptr, nullptr }
};

int
main( int argc, char **argv )
{
    // Option parsing
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
        exit( 0 );
    }

    // setup logging level.
    LOG::SetDefaultLoggerLevel( LOG::WARN );
    if( options[ VERBOSE ] )
        LOG::SetDefaultLoggerLevel( LOG::INFO );
    if( options[ QUIET ] )
        LOG::SetDefaultLoggerLevel( LOG::CHECK );

    // unknown options
    for( option::Option* opt = options[ UNKNOWN ]; opt; opt = opt->next() ){
        DLOG( WARN ) << "Unknown option: " << std::string( opt->name,opt->namelen );
        fail = true;
    }

    // non options
    if( parse.nonOptionsCount() == 0 ){
        LOG( WARN ) << "Missing input filename";
        fail = true;
    }

    for( int i = 1; i < parse.nonOptionsCount(); ++i ){
        LOG( WARN ) << "Superfluous Argument: " << parse.nonOption( i );
        fail = true;
    }

    if( fail || parse.error() ){
        LOG( ERROR ) << "Options parsing";
        exit( 1 );
    }
    // end of options parsing

    SMF *smf;
    if(! ( smf = SMF::open( parse.nonOption(0)) ) ){
        LOG( ERROR ) << "cannot open " << parse.nonOption(0);
        exit( 1 );
    }

    std::fstream file;
    OIIO::ImageBuf *buf; //FIXME is buf really needed here?
    TileMap *tileMap;

    LOG( INFO ) << "Extracting Header Info";
    file.open( "out_Header_Info.txt", std::ios::out );
    file << smf->info();
    file.close();

    LOG( INFO ) << "Extracting height image";
    buf = smf->getHeight();
    buf->write( "out_height.tif", OIIO::TypeUnknown, "tif" );


    LOG( INFO ) << "Extracting type image";
    buf = smf->getType();
    buf->write("out_type.tif", OIIO::TypeUnknown, "tif");

    LOG( INFO ) << "Extracting map image";
    tileMap = smf->getMap();
    file.open("out_tilemap.csv", std::ios::out );
    file << tileMap->toCSV();
    file.close();

    LOG( INFO ) << "Extracting mini image";
    buf = smf->getMini();
    buf->write("out_mini.tif", OIIO::TypeUnknown, "tif");

    LOG( INFO ) << "Extracting metal image";
    buf = smf->getMetal();
    buf->write("out_metal.tif", OIIO::TypeUnknown, "tif");

    LOG( INFO ) << "Extracting featureList";
    file.open( "out_featuretypes.txt", std::ios::out );
    file << smf->getFeatureTypes();
    file.close();

    LOG( INFO ) << "Extracting features";
    file.open( "out_features.csv", std::ios::out );
    file << smf->getFeatures();
    file.close();

    buf = smf->getGrass();
    if( buf ){
        LOG( INFO ) << "Extracting grass image";
        buf->write("out_grass.tif", OIIO::TypeUnknown, "tif");
    }
    return 0;
}
