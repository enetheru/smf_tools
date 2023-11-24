/**
*  \brief Testing generic arguments like quiet/verbose etc.
 */

#include <tclap/ArgBase.h>
#include <tclap/ArgSwitch.h>
#include <tclap/ArgSwitchMulti.h>
#include <tclap/ArgValue.h>
#include <tclap/ArgValueMulti.h>
#include <tclap/ArgUnlabeledValues.h>
#include <tclap/CmdLine.h>
#include <tclap/CmdLineBase.h>
#include <tclap/ConstraintBase.h>
#include <tclap/ConstraintValues.h>
#include <tclap/ContainerBase.h>
#include <tclap/Exception.h>
#include <tclap/GroupBase.h>
#include <tclap/Groups.h>
#include <tclap/Output_fmt.h>
#include <tclap/OutputBase.h>

#include <spdlog/spdlog.h>
#include <spdlog/cfg/env.h>

#include <filesystem>

using namespace TCLAP;
using Path = std::filesystem::path;

static void shutdown( const int code ){
    exit( code );
}

int
main( const int argc, char **argv )
{
    spdlog::set_pattern("[%l] %s:%#:%! | %v");    // Option parsing
    spdlog::set_level( spdlog::level::trace );
    spdlog::cfg::load_env_levels();

    CmdLine cmd("Command description message", ' ', "0.9" );
    const auto argVersion = cmd.create< ArgSwitch >( "V", "version", "Displays version information and exits" );
    const auto argHelp    = cmd.create< ArgSwitch >( "h", "help", "Displays usage information and exits" );
    const auto argQuiet   = cmd.create< ArgSwitch >( "q", "quiet", "Supress Output" );
    const auto argVerbose = cmd.create< ArgSwitchMulti >( "v", "verbose", "MOAR output, multiple times increases output, eg; '-vvvv'" );

    // Parse TCLAP arguments
    std::vector<std::string> args(argc);
    for( int i = 0; i < argc; ++i) args.emplace_back( argv[i] );
    SPDLOG_INFO("argc: {}, argv: {}", argc, fmt::join( args, ", " ) );
    fmtOutput myout;
    try {
        cmd.setOutput( &myout );
        cmd.parse( argc, argv );
    }
    catch( Exception&e ){
        SPDLOG_ERROR( "error: {} for arg {}", e.error(), e.argId() );
        shutdown(1);
    }
    catch( std::exception &e ) {
        SPDLOG_ERROR( "error: {}", e.what() );
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

    shutdown(0);
    return 0;
}