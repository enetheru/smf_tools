/******************************************************************************
 *
 *  file:  Arg.h
 *
 *  Copyright (c) 2003, Michael E. Smoot .
 *  Copyright (c) 2004, Michael E. Smoot, Daniel Aarno .
 *  Copyright (c) 2017 Google Inc.
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

#ifndef TCLAP_ARG_H
#define TCLAP_ARG_H

#ifdef HAVE_TCLAP_CONFIG_H
#include <tclap/TCLAPConfig.h>
#endif

#include <tclap/ArgException.h>

#include <fmt/core.h>
#include <string>
#include <utility>
#include <vector>
#include <functional>

namespace TCLAP {
/**
 * A virtual base class that defines the essential data for all arguments.
 * This class, or one of its existing children, must be subclassed to do
 * anything.
 */

class Arg {
public:
    using Visitor = std::function<void(const Arg&)>;

    /**
        * Prevent accidental copying.
        */
    Arg( const Arg& rhs ) = delete;

    /**
     * Prevent accidental copying.
     */
    Arg& operator=( const Arg& rhs ) = delete;

private:
    /**
     * The delimiter that separates an argument flag/name from the
     * value.
     */
    static constexpr char& delimiterRef() {
        static char delim = ' ';
        return delim;
    }

protected:
    /**
     * The single char flag used to identify the argument.
     * This value (preceded by a dash {-}), can be used to identify
     * an argument on the command line.  The _flag can be blank,
     * in fact this is how unlabeled args work.  Unlabeled args must
     * override appropriate functions to get correct handling. Note
     * that the _flag does NOT include the dash as part of the flag.
     */
    const std::string _flag;

    /**
     * A single word namd identifying the argument.
     * This value (preceded by two dashed {--}) can also be used
     * to identify an argument on the command line.  Note that the
     * _name does NOT include the two dashes as part of the _name. The
     * _name cannot be blank.
     */
    const std::string _name;

    /**
     * Description of the argument.
     */
    const std::string _description;

    /**
     * Indicating whether the argument is required.
     */
    const bool _required{};

    /**
     * Label to be used in usage description.  Normally set to
     * "required", but can be changed when necessary.
     */
    std::string _requireLabel;

    /**
     * Indicates whether a value is required for the argument.
     * Note that the value may be required but the argument/value
     * combination may not be, as specified by _required.
     */
    bool _valueRequired{};

    /**
     * Indicates whether the argument has been set.
     * Indicates that a value on the command line has matched the
     * name/flag of this argument and the values have been set accordingly.
     */
    bool _alreadySet{};

    /** Indicates the value specified to set this flag (like -a or --all).
     */
    std::string _setBy;

    /**
     * A pointer to a visitor object.
     * The visitor allows special handling to occur as soon as the
     * argument is matched.  This defaults to NULL and should not
     * be used unless absolutely necessary.
     */
    Visitor _visitor{};

    bool _acceptsMultipleValues{};

    /**
     * Indicates if the argument is visible in the help output (e.g.,
     * when specifying --help).
     */
    bool _visibleInHelp{};

    /**
     * Performs the special handling described by the Visitor.
     */
    virtual void _visit() const { if( _visitor != nullptr ) _visitor( *this ); }

    /**
     * Primary constructor. YOU (yes you) should NEVER construct an Arg
     * directly, this is a base class that is extended by various children
     * that are meant to be used.  Use SwitchArg, ValueArg, MultiArg,
     * UnlabeledValueArg, or UnlabeledMultiArg instead.
     *
     * \param flag - The flag identifying the argument.
     * \param name - The name identifying the argument.
     * \param desc - The description of the argument, used in the usage.
     * \param req - Whether the argument is required.
     * \param valreq - Whether the a value is required for the argument.
     * \param visitor - The visitor checked by the argument. Defaults to NULL.
     */
    Arg(
        std::string flag,
        std::string name,
        std::string desc,
        bool req = false,
        bool valreq = false,
        Visitor visitor = nullptr );

public:
    /**
     * Destructor.
     */
    virtual ~Arg() = default;

    /**
     * The delimiter that separates an argument flag/name from the
     * value.
     */
    static constexpr char delimiter() { return delimiterRef(); }

    /**
     * The char used as a place holder when SwitchArgs are combined.
     * Currently set to the bell char (ASCII 7).
     */
    static constexpr char blankChar() { return '\a'; }

    /**
     * The char that indicates the beginning of a flag.  Defaults to '-', but
     * clients can define TCLAP_FLAGSTARTCHAR to override.
     */
#ifndef TCLAP_FLAGSTARTCHAR
#define TCLAP_FLAGSTARTCHAR '-'
#endif
    static constexpr char flagStartChar() { return TCLAP_FLAGSTARTCHAR; }

    /**
     * The sting that indicates the beginning of a flag.  Defaults to "-", but
     * clients can define TCLAP_FLAGSTARTSTRING to override. Should be the same
     * as TCLAP_FLAGSTARTCHAR.
     */
#ifndef TCLAP_FLAGSTARTSTRING
#define TCLAP_FLAGSTARTSTRING "-"
#endif
    static constexpr std::string flagStartString() { return TCLAP_FLAGSTARTSTRING; }

    /**
     * The sting that indicates the beginning of a name.  Defaults to "--", but
     *  clients can define TCLAP_NAMESTARTSTRING to override.
     */
#ifndef TCLAP_NAMESTARTSTRING
#define TCLAP_NAMESTARTSTRING "--"
#endif
    static constexpr std::string nameStartString() { return TCLAP_NAMESTARTSTRING; }

    /**
     * Sets the delimiter for all arguments.
     * \param c - The character that delimits flags/names from values.
     */
    static void setDelimiter( const char c ) { delimiterRef() = c; }

    /**
     * Pure virtual method meant to handle the parsing and value assignment
     * of the string on the command line.
     * \param i - Pointer the the current argument in the list.
     * \param args - Mutable list of strings. What is
     * passed in from main.
     */
    virtual bool processArg( int* i, std::vector< std::string >& args ) = 0;

    /**
     * Operator ==.
     * Equality operator. Must be virtual to handle unlabeled args.
     * \param a - The Arg to be compared to this.
     */
    virtual bool operator==( const Arg& a ) const;

    /**
     * Returns the argument flag.
     */
    [[nodiscard]] constexpr const std::string& getFlag() const;

    /**
     * Returns the argument name.
     */
    [[nodiscard]] constexpr const std::string& getName() const;

    /**
     * Returns the argument description.
     */
    [[nodiscard]] constexpr std::string getDescription() const {
     return (_required ? "(" + _requireLabel + ") " : "") + _description;
    }

    /**
     * Indicates whether the argument is required.
     */
    [[nodiscard]] virtual bool isRequired() const;

    /**
     * Indicates whether a value must be specified for argument.
     */
    [[nodiscard]] bool isValueRequired() const;

    /**
     * Indicates whether the argument has already been set.  Only true
     * if the arg has been matched on the command line.
     */
    [[nodiscard]] bool isSet() const;

    /**
     * Returns the value specified to set this flag (like -a or --all).
     */
    [[nodiscard]] const std::string& setBy() const { return _setBy; }

    /**
     * A method that tests whether a string matches this argument.
     * This is generally called by the processArg() method.  This
     * method could be re-implemented by a child to change how
     * arguments are specified on the command line.
     * \param argFlag - The string to be compared to the flag/name to determine
     * whether the arg matches.
     */
    [[nodiscard]] virtual bool argMatches( const std::string& argFlag ) const;

    /**
     * Returns a simple string representation of the argument.
     * Primarily for debugging.
     */
    [[nodiscard]] virtual constexpr std::string toString() const;

    /**
     * Returns a short ID for the usage.
     */
    [[nodiscard]] virtual constexpr std::string shortID() const;

    /**
     * Returns a long ID for the usage.
     */
    [[nodiscard]] virtual constexpr std::string longID( ) const;

    /**
     * Trims a value off of the flag.
     * \param flag - The string from which the flag and value will be
     * trimmed. Contains the flag once the value has been trimmed.
     * \param value - Where the value trimmed from the string will
     * be stored.
     */
    virtual void trimFlag( std::string& flag, std::string& value ) const;

    /**
     * Checks whether a given string has blank chars, indicating that
     * it is a combined SwitchArg.  If so, return true, otherwise return
     * false.
     * \param s - string to be checked.
     */
    static bool _hasBlanks( const std::string& s );

    /**
     * Use by output classes to determine whether an Arg accepts
     * multiple values.
     */
    virtual bool acceptsMultipleValues();

    /**
     * Clears the Arg object and allows it to be reused by new
     * command lines.
     */
    virtual void reset();

    /**
     * Hide this argument from the help output (e.g., when
     * specifying the --help flag or on error.
     */
    virtual void hideFromHelp( const bool hide ) { _visibleInHelp = !hide; }

    /**
     * Returns true if this Arg is visible in the help output.
     */
    [[nodiscard]] virtual bool visibleInHelp() const { return _visibleInHelp; }

    [[nodiscard]] virtual bool hasLabel() const { return true; }
};

inline Arg::Arg(
    std::string flag,
    std::string name,
    std::string desc,
    const bool req,
    const bool valreq,
    Visitor visitor )
    : _flag( std::move( flag ) ),
      _name( std::move( name ) ),
      _description( std::move( desc ) ),
      _required( req ),
      _requireLabel( "required" ),
      _valueRequired( valreq ),
      _alreadySet( false ),
      _visitor(std::move( visitor )),
      _acceptsMultipleValues( false ),
      _visibleInHelp( true ) {
    if( _flag.length() > 1 )
        throw SpecificationException( "Argument flag can only be one character long", Arg::toString() );

    //FIXME, I have moved ignoreNameString out of this class
    /*if( _name != ignoreNameString()
        && (_flag == flagStartString() || _flag == nameStartString() || _flag == " ") ) {
        throw SpecificationException(
            std::format( "Argument flag cannot be either '{}' or '{}' or a space.", flagStartString(),
                         nameStartString() ),
            Arg::toString() );
    }*/

    if( _name.substr( 0, flagStartString().length() ) == flagStartString() ||
        _name.substr( 0, nameStartString().length() ) == nameStartString() ||
        _name.find( ' ', 0 ) != std::string::npos )
        throw SpecificationException(
            fmt::format( "Argument name begin with either '{}' or '{}' or space.",
                             flagStartString(), nameStartString() ),
            Arg::toString() );
}

constexpr std::string Arg::shortID() const {
    std::string id;

    if( !_flag.empty() ) id = flagStartString() + _flag;
    else id = nameStartString() + _name;

    return id;
}

constexpr std::string Arg::longID() const {
    const std::string id{ !_flag.empty() ? flagStartString() + _flag + ",  " : "" };
    return id + nameStartString() + _name;
}

inline bool Arg::operator==( const Arg& a ) const {
    if( (!_flag.empty() && _flag == a._flag) || _name == a._name ) return true;
    return false;
}

constexpr const std::string& Arg::getFlag() const { return _flag; }

constexpr const std::string& Arg::getName() const { return _name; }

inline bool Arg::isRequired() const { return _required; }

inline bool Arg::isValueRequired() const { return _valueRequired; }

inline bool Arg::isSet() const { return _alreadySet; }

inline bool Arg::argMatches( const std::string& argFlag ) const {
    if( (argFlag == flagStartString() + _flag && !_flag.empty()) ||
        argFlag == nameStartString() + _name )
        return true;
    return false;
}

constexpr std::string Arg::toString() const {
    if( _flag.empty() ) {
        return fmt::format("{}{}", nameStartString(), _name);
    }
    return fmt::format("{}{}, ({}{})", flagStartString(), _flag, nameStartString(), _name);
}

/**
 * Implementation of trimFlag.
 */
inline void Arg::trimFlag( std::string& flag, std::string& value ) const {
    int stop = 0;
    for( int i = 0; static_cast< unsigned int >(i) < flag.length(); i++ )
        if( flag[ i ] == delimiter() ) {
            stop = i;
            break;
        }

    if( stop > 1 ) {
        value = flag.substr( stop + 1 );
        flag = flag.substr( 0, stop );
    }
}

/**
 * Implementation of _hasBlanks.
 */
inline bool Arg::_hasBlanks( const std::string& s ) {
    for( int i = 1; static_cast< unsigned int >(i) < s.length(); i++ )
        if( s[ i ] == blankChar() ) return true;

    return false;
}

inline bool Arg::acceptsMultipleValues() { return _acceptsMultipleValues; }

inline void Arg::reset() { _alreadySet = false; }

//////////////////////////////////////////////////////////////////////
// END Arg.cpp
//////////////////////////////////////////////////////////////////////
} // namespace TCLAP

#endif  // TCLAP_ARG_H
