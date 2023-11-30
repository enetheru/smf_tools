/******************************************************************************
 *
 *  file:  SwitchArg.h
 *
 *  Copyright (c) 2003, Michael E. Smoot .
 *  Copyright (c) 2004, Michael E. Smoot, Daniel Aarno.
 *  Copyright (c) 2017, Google LLC
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

#ifndef TCLAP_SWITCH_ARG_H
#define TCLAP_SWITCH_ARG_H

#include <tclap/ArgBase.h>

#include <string>
#include <vector>

namespace TCLAP {
/**
 * A simple switch argument.  If the switch is set on the command line, then
 * the getValue method will return the opposite of the default value for the
 * switch.
 */
class ArgSwitch : public ArgBase {
protected:
    /**
     * The value of the switch.
     */
    bool _value;

    /**
     * Used to support the reset() method so that ValueArg can be
     * reset to their constructed value.
     */
    bool _default;

public:
    /**
     * SwitchArg constructor.
     * \param flag - The one character flag that identifies this
     * argument on the command line.
     * \param name - A one word name for the argument.  Can be
     * used as a long flag on the command line.
     * \param desc - A description of what the argument is for or
     * does.
     * \param default_val - The default value for this Switch.
     * \param visitor - An optional visitor.  You probably should not
     * use this unless you have a very good reason.
     */
    ArgSwitch( const std::string& flag, const std::string& name, const std::string& desc,
        bool default_val = false, const Visitor& visitor = {} );

    /**
     * Handles the processing of the argument.
     * This re-implements the Arg version of this method to set the
     * _value of the argument appropriately.
     * \param i - Pointer the the current argument in the list.
     * \param args - Mutable list of strings. Passed
     * in from main().
     */
    bool processArg( int* i, std::vector< std::string >& args ) override;

    /**
     * Checks a string to see if any of the chars in the string
     * match the flag for this Switch.
     */
    bool combinedSwitchesMatch( std::string& combinedSwitches );

    /**
     * Returns bool, whether or not the switch has been set.
     */
    // ReSharper disable once CppHiddenFunction
    [[nodiscard]] bool getValue() const { return _value; }

    /**
     * A SwitchArg can be used as a boolean, indicating
     * whether or not the switch has been set. This is the
     * same as calling getValue()
     */
    // ReSharper disable once CppNonExplicitConversionOperator
    explicit operator bool() const { return _value; }

    void reset() override;

private:
    /**
     * Checks to see if we've found the last match in
     * a combined string.
     */
    static bool lastCombined( const std::string& combinedSwitches );

    /**
     * Does the common processing of processArg.
     */
    void commonProcessing();
};

//////////////////////////////////////////////////////////////////////
// BEGIN SwitchArg.cpp
//////////////////////////////////////////////////////////////////////
inline ArgSwitch::ArgSwitch( const std::string& flag, const std::string& name,
                             const std::string& desc, const bool default_val,
                             const Visitor& visitor )
    : ArgBase( flag, name, desc, false, false, visitor ),
      _value( default_val ),
      _default( default_val ) {}

inline bool ArgSwitch::lastCombined( const std::string& combinedSwitches ) {
    for( unsigned int i = 1; i < combinedSwitches.length(); i++ )
        if( combinedSwitches[ i ] != blankChar() ) return false;

    return true;
}

inline bool ArgSwitch::combinedSwitchesMatch( std::string& combinedSwitches ) {
    // make sure this is actually a combined switch
    if( !combinedSwitches.empty() &&
        combinedSwitches[ 0 ] != flagStartString()[ 0 ] )
        return false;

    // make sure it isn't a long name
    if( combinedSwitches.substr( 0, nameStartString().length() ) == nameStartString() )
        return false;

    // make sure the delimiter isn't in the string
    if( combinedSwitches.find_first_of( delimiter() ) != std::string::npos )
        return false;

    // ok, we're not specifying a ValueArg, so we know that we have
    // a combined switch list.
    for( unsigned int i = 1; i < combinedSwitches.length(); i++ ) {
        if( !_flag.empty() && combinedSwitches[ i ] == _flag[ 0 ] &&
            _flag[ 0 ] != flagStartString()[ 0 ] ) {
            // update the combined switches so this one is no longer
            // present this is necessary so that no unlabeled args are
            // matched later in the processing.
            // combinedSwitches.erase(i,1);
            _setBy = flagStartString() + combinedSwitches[ i ];
            combinedSwitches[ i ] = blankChar();
            return true;
        }
    }

    // none of the switches passed in the list match.
    return false;
}

inline void ArgSwitch::commonProcessing() {
    if( _isSet )
        throw CmdLineParseException( "Argument already set!", toString() );

    _isSet = true;

    if( _value == true )
        _value = false;
    else
        _value = true;

    _visit();
}

inline bool ArgSwitch::processArg( int* i, std::vector< std::string >& args ) {
    if( argMatches( args[ *i ] ) ) {
        // The whole string matches the flag or name string
        _setBy = args[ *i ];
        commonProcessing();

        return true;
    }
    if( combinedSwitchesMatch( args[ *i ] ) ) {
        // A substring matches the flag as part of a combination
        // check again to ensure we don't misinterpret
        // this as a MultiSwitchArg
        if( combinedSwitchesMatch( args[ *i ] ) )
            throw CmdLineParseException( "Argument already set!", toString() );

        commonProcessing();

        // We only want to return true if we've found the last combined
        // match in the string, otherwise we return true so that other
        // switches in the combination will have a chance to match.
        return lastCombined( args[ *i ] );
    }

    return false;
}

inline void ArgSwitch::reset() {
    ArgBase::reset();
    _value = _default;
}
} // namespace TCLAP

#endif  // TCLAP_SWITCH_ARG_H