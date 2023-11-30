#include <spdlog/spdlog.h>
#include <tclap/CmdLine.h>
#include <tclap/Output_fmt.h>
#include <tclap/ArgSwitchMulti.h>
#include <tclap/ArgValue.h>

#include "args.h"
#include "smflib/basicio.h"
#include "smflib/smf.h"

using namespace TCLAP;
using component = smflib::SMF::SMFComponent;

static void shutdown( const int code ){
    exit( code );
}

int
main( const int argc, char **argv )
{
    spdlog::set_pattern("[%l] %s:%#:%! | %v");    // Option parsing
    spdlog::set_level( spdlog::level::warn );

    const auto heightIO = std::make_shared< smflib::BasicHeightIo >();
    const auto typeIO   = std::make_shared<smflib::BasicTypeIO>();
    const auto miniIO   = std::make_shared<smflib::BasicMiniIO>();
    const auto metalIO   = std::make_shared<smflib::BasicMetalIO>();
    const auto grassIO   = std::make_shared<smflib::BasicGrassIO>();
    const auto tileIO   = std::make_shared<smflib::BasicTileIO>();
    const auto featureIO   = std::make_shared<smflib::BasicFeatureIO>();

    smflib::SMF smf;
    smf.setHeightIO( heightIO );
    smf.setTypeIO( typeIO );
    smf.setMiniIO( miniIO );
    smf.setMetalIO( metalIO );
    smf.setGrassIO( grassIO );
    smf.setTileIO( tileIO );
    smf.setFeatureIO( featureIO );

    CmdLine cmd("Command description message", ' ', "0.9" );
    const auto argVersion = cmd.create< ArgSwitch >( "V", "version", "Displays version information and exits" );
    const auto argHelp    = cmd.create< ArgSwitch >( "h", "help", "Displays usage information and exits" );
    const auto argQuiet   = cmd.create< ArgSwitch >( "q", "quiet", "Supress Output" );
    const auto argVerbose = cmd.create< ArgSwitchMulti >( "v", "verbose", "MOAR output, multiple times increases output, eg; '-vvvv'" );

    // Arguments specific to smf_info
    const auto argPath = cmd.create<ArgValue<Path>>("f","file"," Path of file to analyse", false, Path{} ,"path", std::make_shared<IsFile_SMF>() ); //TODO add constraint
    // FIXME, add an unlabeled argument specifier
    // FIXME, create a MultiArgValue argument for specifying multiple values

    // Parse TCLAP arguments
    fmtOutput myout;
    try {
        cmd.setOutput( &myout );
        cmd.parse( argc, argv );
    }
    catch( Exception&e ){
        fmt::println(stderr, "error: {} for arg {}", e.error(), e.argId() );
        shutdown(1);
    }

    if( cmd.getNumMatched() == 0 ) {
        //FIXME output short help when no arguments are supplied
        SPDLOG_ERROR( "No arguments were supplied" );
        shutdown(1);
    }

    // Deal with basic arguments
    if( argHelp->isSet() ) {
        myout.usage(cmd);
        shutdown(0);
    }

    if( argVersion->isSet() ) {
        myout.version(cmd);
        shutdown(0);
    }

    // setup logging level.
    int verbose = 3;
    if( argQuiet->getValue() ) verbose = 0;
    verbose += argVerbose->getValue();
    if( verbose > 6 )verbose = 6;

    switch( verbose ){
    case 0: spdlog::set_level( spdlog::level::off );
        break;
    case 1: spdlog::set_level( spdlog::level::critical );
        break;
    case 2: spdlog::set_level( spdlog::level::err );
        break;
    case 3: spdlog::set_level( spdlog::level::warn );
        break;
    case 4: spdlog::set_level( spdlog::level::info );
        break;
    case 5: spdlog::set_level( spdlog::level::debug );
        break;
    case 6: spdlog::set_level( spdlog::level::trace );
        break;
    default: spdlog::set_level( spdlog::level::warn );
        break;
    }
    std::array levels SPDLOG_LEVEL_NAMES;
    if( verbose >=4 ) println( "Spdlog Level: {}", levels[spdlog::get_level()] );

    if( argPath->isSet() ) {
        smf.set_file_path( argPath->getValue() );
        smf.read(component::ALL );

        SPDLOG_INFO( "Getting info for file: {}", argPath->getValue().string() );
        fmt::println( "{}\n{}", argPath->getValue().string(), smf.json().dump(4) );
    }

    shutdown(0);
    return 0;
}