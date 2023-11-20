/******************************************************************************
 *
 *  file:  MultiSwitchArg.h
 *
 *  Copyright (c) 2003, Michael E. Smoot .
 *  Copyright (c) 2004, Michael E. Smoot, Daniel Aarno.
 *  Copyright (c) 2005, Michael E. Smoot, Daniel Aarno, Erik Zeek.
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

#ifndef TCLAP_MULTI_SWITCH_ARG_H
#define TCLAP_MULTI_SWITCH_ARG_H

#include <tclap/SwitchArg.h>

#include <string>
#include <utility>
#include <vector>

namespace TCLAP {
/**
 * A multiple switch argument.  If the switch is set on the command line, then
 * the getValue method will return the number of times the switch appears.
 */
class MultiSwitchArg final : public SwitchArg {
protected:
    /**
     * The value of the switch.
     */
    int _value;

    /**
     * Used to support the reset() method so that ValueArg can be
     * reset to their constructed value.
     */
    int _default;

public:
    /**
     * MultiSwitchArg constructor.
     * \param flag - The one character flag that identifies this
     * argument on the command line.
     * \param name - A one word name for the argument.  Can be
     * used as a long flag on the command line.
     * \param desc - A description of what the argument is for or
     * does.
     * \param init - Optional. The initial/default value of this Arg.
     * Defaults to 0.
     * \param v - An optional visitor.  You probably should not
     * use this unless you have a very good reason.
     */
    MultiSwitchArg( const std::string& flag, const std::string& name,
                    const std::string& desc, int init = 0, Visitor v = nullptr );

    /**
     * MultiSwitchArg constructor.
     * \param flag - The one character flag that identifies this
     * argument on the command line.
     * \param name - A one word name for the argument.  Can be
     * used as a long flag on the command line.
     * \param desc - A description of what the argument is for or
     * does.
     * \param parser - A CmdLine parser object to add this Arg to
     * \param init - Optional. The initial/default value of this Arg.
     * Defaults to 0.
     * \param v - An optional visitor.  You probably should not
     * use this unless you have a very good reason.
     */
    MultiSwitchArg( const std::string& flag, const std::string& name,
                    const std::string& desc, ArgContainer& parser, int init = 0,
                    Visitor* v = nullptr );

    /**
     * Handles the processing of the argument.
     * This re-implements the SwitchArg version of this method to set the
     * _value of the argument appropriately.
     * \param i - Pointer the the current argument in the list.
     * \param args - Mutable list of strings. Passed
     * in from main().
     */
    bool processArg( int* i, std::vector< std::string >& args ) override;

    /**
     * Returns int, the number of times the switch has been set.
     */
    // ReSharper disable once CppHidingFunction
    [[nodiscard]] int getValue() const { return _value; }

    /**
     * Returns the shortID for this Arg.
     */
    [[nodiscard]] std::string shortID() const override;

    /**
     * Returns the longID for this Arg.
     */
    [[nodiscard]] std::string longID() const override;

    void reset() override;
};

inline MultiSwitchArg::MultiSwitchArg( const std::string& flag,
                                       const std::string& name,
                                       const std::string& desc,
                                       const int init,
                                       const Visitor v )
    : SwitchArg( flag, name, desc, false, v ), _value( init ), _default( init ) {}

inline bool MultiSwitchArg::processArg( int* i, std::vector< std::string >& args ) {
    if( argMatches( args[ *i ] ) ) {
        // so the isSet() method will work
        _alreadySet = true;
        _setBy = args[ *i ];

        // Matched argument: increment value.
        ++_value;

        _visit();

        return true;
    }
    if( combinedSwitchesMatch( args[ *i ] ) ) {
        // so the isSet() method will work
        _alreadySet = true;

        // Matched argument: increment value.
        ++_value;

        // Check for more in argument and increment value.
        while( combinedSwitchesMatch( args[ *i ] ) ) ++_value;

        _visit();

        return false;
    }
    return false;
}

inline std::string MultiSwitchArg::shortID() const {
    return Arg::shortID() + " ...";
}

inline std::string MultiSwitchArg::longID() const {
    return Arg::longID() + "  (accepted multiple times)";
}

inline void MultiSwitchArg::reset() {
    _value = _default;
}
} // namespace TCLAP

#endif  // TCLAP_MULTI_SWITCH_ARG_H