/******************************************************************************
 *
 *  file:  CmdLine.h
 *
 *  Copyright (c) 2003, Michael E. Smoot .
 *  Copyright (c) 2004, Michael E. Smoot, Daniel Aarno.
 *  Copyright (c) 2018, Google LLC
 *  All rights reserved.
 *
 *  See the file COPYING in the top directory of this distribution for
 *  more information.
 *
 *  THE SOFTWARE IS PROVIDED _AS IS_, WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 *
 *****************************************************************************/

#ifndef TCLAP_CMD_LINE_H
#define TCLAP_CMD_LINE_H

#include <tclap/SwitchArg.h>

#include <tclap/IgnoreRestVisitor.h>

#include <tclap/CmdLineOutput.h>

#include <tclap/ArgGroup.h>
#include <tclap/DeferDelete.h>

#include <cstdlib>
#include <list>
#include <string>
#include <utility>
#include <vector>
#include <filesystem>

namespace TCLAP {
class StandaloneArgs final : public AnyOf {
public:
    StandaloneArgs() = default;

    ArgContainer& add( const std::shared_ptr<Arg> arg ) override {
        for( const auto& it : *this ) {
            if( *arg == *it ) {
                throw SpecificationException(
                    "Argument with same flag/name already exists!",
                    arg->longID() );
            }
        }

        push_back( arg );
        return *this;
    }

    [[nodiscard]] bool showAsGroup() const override { return false; }
};

/**
 * The base class that manages the command line definition and passes
 * along the parsing to the appropriate Arg classes.
 */
class CmdLine final : public CmdLineInterface {

    /**
     * The name used to identify the ignore rest argument.
     */
    static std::string ignoreNameString() { return "ignore_rest"; }

protected:
    /**
     * The list of arguments that will be tested against the
     * command line.
     */
    std::list< std::shared_ptr<Arg> > _argList;

    StandaloneArgs _standaloneArgs;

    /**
     * Some args have set constraints on them (i.e., exactly or at
     * most one must be specified.
     */
    std::list< ArgGroup* > _argGroups;

    /**
     * The name of the program.  Set to argv[0].
     */
    std::string _progName;

    /**
     * A message used to describe the program.  Used in the usage output.
     */
    std::string _message;

    /**
     * The version to be displayed with the --version switch.
     */
    std::string _version;

    /**
     * The number of arguments that are required to be present on
     * the command line. This is set dynamically, based on the
     * Args added to the CmdLine object.
     */
    int _numRequired;

    /**
     * The character that is used to separate the argument flag/name
     * from the value.  Defaults to ' ' (space).
     */
    char _delimiter;

    /**
     * Add pointers that should be deleted as part of cleanup when
     * this object is destroyed.
     * @internal.
     */
    DeferDelete _deleteOnExit;

    /**
     * Object that handles all output for the CmdLine.
     */
    CmdLineOutput* _output;

    /**
     * Should CmdLine handle parsing exceptions internally?
     */
    bool _handleExceptions;

    /**
     * Throws an exception listing the missing args.
     */
    void missingArgsException( const std::list< ArgGroup* >& missing ) const;

    /**
     * Checks whether a name/flag string matches entirely matches
     * the Arg::blankChar.  Used when multiple switches are combined
     * into a single argument.
     * \param s - The message to be used in the usage.
     */
    static bool _emptyCombined( const std::string& s );

public:
    /**
     * Prevent accidental copying.
     */
    CmdLine( const CmdLine& rhs ) = delete;
    CmdLine& operator=( const CmdLine& rhs ) = delete;

private:
    /**
     * Encapsulates the code common to the constructors
     * (which is all of it).
     */
    void _constructor();

    /**
     * Whether or not to automatically create help and version switches.
     */
    bool _helpAndVersion;

    /**
     * Whether or not to ignore unmatched args.
     */
    bool _ignoreUnmatched;

    /**
     * Ignoring arguments (e.g., after we have seen "--")
     */
    bool _ignoring;

public:
    /**
     * Command line constructor. Defines how the arguments will be
     * parsed.
     * \param message - The message to be used in the usage
     * output.
     * \param delimiter - The character that is used to separate
     * the argument flag/name from the value.  Defaults to ' ' (space).
     * \param version - The version number to be used in the
     * --version switch.
     * \param helpAndVersion - Whether or not to create the Help and
     * Version switches. Defaults to true.
     */
    explicit CmdLine( std::string message,
                      char delimiter = ' ',
                      std::string version = "none",
                      bool helpAndVersion = true );

    /**
     * Deletes any resources allocated by a CmdLine object.
     */
    ~CmdLine() override = default;

    /**
     * Adds an argument to the list of arguments to be parsed.
     *
     * @param a - Argument to be added.
     * @retval A reference to this so that add calls can be chained
     */
    ArgContainer& add( std::shared_ptr<Arg> a ) override { return *this; }

    template< class ArgType, typename ...args >
    std::shared_ptr<ArgType> add( args  ... fwd );

    /**
     * Adds an argument group to the list of arguments to be parsed.
     *
     * All arguments in the group are added and the ArgGroup
     * object will validate that the input matches its
     * constraints.
     *
     * @param args - Argument group to be added.
     * @retval A reference to this so that add calls can be chained
     */
    ArgContainer& add( ArgGroup& args ) override;

    // Internal, do not use
    void addToArgList( std::shared_ptr<Arg> a ) override;

    /**
     * Parses the command line.
     * \param argc - Number of arguments.
     * \param argv - Array of arguments.
     */
    void parse( int argc, const char* const * argv ) override;

    /**
     * Parses the command line.
     * \param args - A vector of strings representing the args.
     * args[0] is still the program name.
     */
    // ReSharper disable once CppHidingFunction
    void parse( std::vector< std::string >& args );

    void setOutput( CmdLineOutput* co ) override;

    [[nodiscard]] std::string getVersion() const override { return _version; }

    [[nodiscard]] std::string getProgramName() const override { return _progName; }

    // TOOD: Get rid of getArgList
    [[nodiscard]] std::list< std::shared_ptr<Arg> > getArgList() const override { return _argList; }

    std::list< ArgGroup* > getArgGroups() override {
        std::list< ArgGroup* > groups = _argGroups;
        return groups;
    }

    [[nodiscard]] char getDelimiter() const override { return _delimiter; }
    [[nodiscard]] std::string getMessage() const override { return _message; }
    [[nodiscard]] bool hasHelpAndVersion() const override { return _helpAndVersion; }

    /**
     * Disables or enables CmdLine's internal parsing exception handling.
     *
     * @param state Should CmdLine handle parsing exceptions internally?
     */
    void setExceptionHandling( bool state );

    /**
     * Returns the current state of the internal exception handling.
     *
     * @retval true Parsing exceptions are handled internally.
     * @retval false Parsing exceptions are propagated to the caller.
     */
    [[nodiscard]] bool hasExceptionHandling() const { return _handleExceptions; }

    /**
     * Allows the CmdLine object to be reused.
     */
    void reset() override;

    /**
     * Allows unmatched args to be ignored. By default false.
     *
     * @param ignore If true the cmdline will ignore any unmatched args
     * and if false it will behave as normal.
     */
    void ignoreUnmatched( bool ignore );

    void beginIgnoring() override { _ignoring = true; }
    bool ignoreRest() override { return _ignoring; }
};

///////////////////////////////////////////////////////////////////////////////
// Begin CmdLine.cpp
///////////////////////////////////////////////////////////////////////////////

inline CmdLine::CmdLine( std::string message, const char delimiter, std::string version, const bool helpAndVersion )
    : _progName( "not_set_yet" ), _message( std::move( message ) ),
      _version( std::move( version ) ), _numRequired( 0 ), _delimiter( delimiter ), _output(),
      _handleExceptions( true ), _helpAndVersion( helpAndVersion ), _ignoreUnmatched( false ), _ignoring( false ) {
    _constructor();
}

inline void CmdLine::_constructor() {
    Arg::setDelimiter( _delimiter );

    add( _standaloneArgs );

    auto v = std::make_shared<IgnoreRestVisitor>( *this );
    const auto ignore = std::make_shared<SwitchArg>( Arg::flagStartString(), ignoreNameString(),
        "Ignores the rest of the labeled arguments following this flag.", false,
        v );

    addToArgList( ignore );
}

template< class ArgType, typename ...args >
std::shared_ptr<ArgType> CmdLine::add( args  ... fwd ) {
    auto item = std::make_shared<ArgType>( std::move(fwd) ... );
    addToArgList( item );
    _standaloneArgs.add( item );
    return item;
}

inline ArgContainer& CmdLine::add( ArgGroup& args ) {
    args.setParser( *this );
    _argGroups.push_back( &args );

    return *this;
}


// TODO: Rename this to something smarter or refactor this logic so
// it's not needed.
inline void CmdLine::addToArgList( const std::shared_ptr<Arg> a ) {
    for( const auto &arg : _argList ) {
        if( *a == *arg ) {
            throw SpecificationException(
                "Argument with same flag/name already exists!", a->longID() );
        }
    }

    _argList.push_front( a );
    if( a->isRequired() ) _numRequired++;
}

inline void CmdLine::parse( const int argc, const char* const * argv ) {
    // this step is necessary so that we have easy access to
    // mutable strings.
    std::vector< std::string > args;
    for( int i = 0; i < argc; i++ ) args.emplace_back( argv[ i ] );

    parse( args );
}

inline void CmdLine::parse( std::vector< std::string >& args ) {
    bool shouldExit = false;
    int estat = 0;

    try {
        if( args.empty() ) {
            // https://sourceforge.net/p/tclap/bugs/30/
            throw CmdLineParseException(
                "The args vector must not be empty, the first entry should contain the program's name." );
        }

        // TODO(macbishop): Maybe store the full name somewhere?
        _progName = std::filesystem::path( args.front() ).stem().string();
        args.erase( args.begin() );

        int requiredCount = 0;
        std::list< ArgGroup* > missingArgGroups;

        // Check that the right amount of arguments are provided for
        // each ArgGroup, and if there are any required arguments
        // missing, store them for later. Other errors will cause an
        // exception to be thrown and parse will exit early.
        /*
        for (std::list<ArgGroup*>::iterator it = _argGroups.begin();
             it != _argGroups.end(); ++it) {
            bool missingRequired = (*it)->validate(args);
            if (missingRequired) {
                missingArgGroups.push_back(*it);
            }
        }
        */

        for( int i = 0; static_cast< unsigned int >(i) < args.size(); i++ ) {
            bool matched = false;
            for( const auto &arg : _argList ) {
                // We check if the argument was already set (e.g., for
                // a Multi-Arg) since then we don't want to count it
                // as required again. This is a hack/workaround to
                // make isRequired() imutable so it can be used to
                // display help correctly (also it's a good idea).
                //
                // TODO: This logic should probably be refactored to remove this logic from here.
                const bool alreadySet = arg->isSet();
                if( const bool ignore = !arg->isRequired() && ignoreRest(); !ignore && arg->processArg( &i, args ) ) {
                    requiredCount += !alreadySet && arg->isRequired() ? 1 : 0;
                    matched = true;
                    break;
                }
            }

            // checks to see if the argument is an empty combined
            // switch and if so, then we've actually matched it
            if( !matched && _emptyCombined( args[ i ] ) ) matched = true;

            if( !matched && !ignoreRest() && !_ignoreUnmatched )
                throw CmdLineParseException( "Couldn't find match for argument", args[ i ] );
        }

        // Once all arguments have been parsed, check that we don't
        // violate any constraints.
        for( const auto argGroup : _argGroups ) {
            if( argGroup->validate() ) {
                missingArgGroups.push_back( argGroup );
            }
        }

        if( requiredCount < _numRequired || !missingArgGroups.empty() ) {
            missingArgsException( missingArgGroups );
        }

        if( requiredCount > _numRequired ) {
            throw CmdLineParseException( "Too many arguments!" );
        }
    }
    catch( ArgException& e ) {
        // If we're not handling the exceptions, rethrow.
        if( !_handleExceptions ) {
            throw;
        }

        try {
            _output->failure( *this, e );
        }
        catch( ExitException& ee ) {
            estat = ee.getExitStatus();
            shouldExit = true;
        }
    } catch( ExitException& ee ) {
        // If we're not handling the exceptions, rethrow.
        if( !_handleExceptions ) {
            throw;
        }

        estat = ee.getExitStatus();
        shouldExit = true;
    }

    if( shouldExit ) exit( estat );
}

inline bool CmdLine::_emptyCombined( const std::string& s ) {
    if( !s.empty() && s[ 0 ] != Arg::flagStartChar() ) return false;

    for( unsigned int i = 1; i < s.length(); i++ )
        if( s[ i ] != Arg::blankChar() ) return false;

    return true;
}

inline void CmdLine::missingArgsException(
    const std::list< ArgGroup* >& missing ) const {
    int count = 0;

    std::string missingArgList;
    for( const auto &arg : _argList ) {
        if( arg->isRequired() && !arg->isSet() ) {
            missingArgList += arg->getName();
            missingArgList += ", ";
            count++;
        }
    }

    for( const auto argGroup : missing ) {
        missingArgList += argGroup->getName();
        missingArgList += ", ";
        count++;
    }

    missingArgList = missingArgList.substr( 0, missingArgList.length() - 2 );

    std::string msg = count > 1 ? "Required arguments missing: " : "Required argument missing: ";
    msg += missingArgList;

    throw CmdLineParseException( msg );
}

inline void CmdLine::setOutput( CmdLineOutput* co ) { _output = co; }

inline void CmdLine::setExceptionHandling( const bool state ) {
    _handleExceptions = state;
}

inline void CmdLine::reset() {
    // TODO: This is no longer correct (or perhaps we don't need "reset")
    for( const auto &arg : _argList ) arg->reset();
    _progName.clear();
}

inline void CmdLine::ignoreUnmatched( const bool ignore ) {
    _ignoreUnmatched = ignore;
}

///////////////////////////////////////////////////////////////////////////////
// End CmdLine.cpp
///////////////////////////////////////////////////////////////////////////////
} // namespace TCLAP

#endif  // TCLAP_CMD_LINE_H
