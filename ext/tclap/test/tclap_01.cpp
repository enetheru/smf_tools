/**
 *  \brief Testing all the argument types
 */

#include <tclap/ArgBase.h>
#include <tclap/ArgSwitch.h>
#include <tclap/ArgSwitchMulti.h>
#include <tclap/ArgValue.h>
#include <tclap/ArgValueMulti.h>
#include <tclap/ArgUnlabeledValues.h>
#include <tclap/CmdLine.h>
#include <tclap/ConstraintValues.h>
#include <tclap/Exception.h>
#include <tclap/Output_fmt.h>
#include <tclap/OutputBase.h>

#include <spdlog/spdlog.h>
#include <spdlog/cfg/env.h>

#include <filesystem>
using namespace TCLAP;

int
main( const int argc, char **argv )
{
    spdlog::set_pattern("[%l] %s:%#:%! | %v");    // Option parsing
    spdlog::set_level( spdlog::level::trace );
    spdlog::cfg::load_env_levels();
    std::vector<std::string> args(argc);
    for( int i = 0; i < argc; ++i) {
        args.emplace_back( argv[i] );
    }
    SPDLOG_INFO("argc: {}, argv: {}", argc, fmt::join( args, ", " ) );

    CmdLine cmd("Command description message", ' ', "0.9" );
    cmd.create< ArgSwitch >( "1", "switch", "ArgSwitch - flag indicating true, or on" );
    cmd.create< ArgSwitchMulti >( "2", "switch-multi", "ArgSwitchMulti - flag that can be specified multiple times" );
    cmd.create< ArgValue<int> >( "3", "value", "ArgValue - takes a single parameter" );
    cmd.create< ArgValueMulti<int> >( "4", "value-multi", "ArgValueMulti - takes a multiple parameters, or can be specified multiple times" );
    cmd.create< ArgUnlabeledValues<int> >( "unlabeled-values", "ArgUnlabeledValues - " );

    cmd.create< ArgValueMulti<std::string> >( "a", "a-name", "ArgValueMulti - takes a multiple parameters, or can be specified multiple times" );

    // Parse TCLAP arguments
    fmtOutput myout;
    try {
        cmd.setOutput( &myout );
        cmd.parse( args );
    }
    catch( Exception&e ){
        SPDLOG_ERROR( "error: {} for arg {}", e.error(), e.argId() );
        return 1;
    }
    catch( std::exception &e ) {
        SPDLOG_ERROR( "error: {}", e.what() );
        return 1;
    }

    if( cmd.getNumMatched() == 0 ) {
        //FIXME output short help when no arguments are supplied
        SPDLOG_ERROR( "No arguments were supplied" );
        return 1;
    }


    return 0;
}