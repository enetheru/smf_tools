/******************************************************************************
 *
 *  file:  ValueArg.h
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

#ifndef TCLAP_VALUE_ARG_H
#define TCLAP_VALUE_ARG_H

#include <tclap/ArgBase.h>
#include <tclap/ConstraintBase.h>

#include <string>
#include <utility>
#include <vector>
#include <filesystem>
#include <spdlog/spdlog.h>

/*template <typename T>
constexpr auto type_name() {
    std::string_view name, prefix, suffix;
#ifdef __clang__
    name = __PRETTY_FUNCTION__;
    prefix = "auto type_name() [T = ";
    suffix = "]";
#elif defined(__GNUC__)
    name = __PRETTY_FUNCTION__;
    prefix = "constexpr auto type_name() [with T = ";
    suffix = "]";
#elif defined(_MSC_VER)
    name = __FUNCSIG__;
    prefix = "auto __cdecl type_name<";
    suffix = ">(void)";
#endif
    name.remove_prefix(prefix.size());
    name.remove_suffix(suffix.size());
    return name;
}*/

template <typename T>
constexpr auto type_name() {
    return typeid(T).name();
}

namespace TCLAP {

template<class T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
T ExtractValue( const std::string& val ) {
    return std::stoi( val );
}

template<class T, std::enable_if_t<std::is_floating_point_v<T>, bool> = true>
T ExtractValue( const std::string& val ) { return std::stof( val ); }

template<class T, std::enable_if_t<std::is_convertible_v<T, std::string>, bool> = true>
T ExtractValue( const std::string& val ) { return std::string( val ); }

template<class T, std::enable_if_t<std::is_same_v<T, std::filesystem::path>, bool> = true>
T ExtractValue( const std::string& val ) { return std::filesystem::path( val ); }

/**
 * The basic labeled argument that parses a value.
 * This is a template class, which means the type T defines the type
 * that a given object will attempt to parse when the flag/name is matched
 * on the command line.  While there is nothing stopping you from creating
 * an unflagged ValueArg, it is unwise and would cause significant problems.
 * Instead use an UnlabeledValueArg.
 */
template< class T >
class ArgValue : public ArgBase {
public:
    using Visitor = std::function<void(const ArgValue&)>;
    using Constraint = std::shared_ptr<ConstraintBase< T >>;
protected:
    /**
     * The value parsed from the command line.
     * Can be of any type, as long as the >> operator for the type
     * is defined.
     */
    T _value;

    /**
     * Used to support the reset() method so that ValueArg can be
     * reset to their constructed value.
     */
    T _default;

    /**
     * A human readable description of the type to be parsed.
     * This is a hack, plain and simple.  Ideally we would use RTTI to
     * return the name of type T, but until there is some sort of
     * consistent support for human readable names, we are left to our
     * own devices.
     */
    std::string _typeDesc = type_name<T>();

    /**
     * A Constraint this Arg must conform to.
     */
    Constraint _constraint;

    /**
     * std::function visitor, is called after validation succeeds.
     */
    Visitor _visitor{};

    /**
     * Performs the special handling described by the Visitor.
     */
    void _visit() const override { if(_visitor)_visitor( *this ); }

    /**
     * Extracts the value from the string.
     * Attempts to parse string as type T, if this fails an exception
     * is thrown.
     * \param val - value to be parsed.
     */

    virtual void _extractValue( const std::string& val ) {
        _value = ExtractValue<T>( val );
        if( _constraint ){
            auto [passed, msg] = _constraint->check( *this );
            if( passed != SUCCESS ) throw CmdLineParseException( msg, toString() );
        }
    }

public:
    /**
     * \param flag - The one character flag that identifies this
     *      argument on the command line.
     * \param name - A one word name for the argument.  Can be
     *      used as a long flag on the command line.
     * \param desc - A description of what the argument is for or
     *      does.
     * \param constraint -
     * \param visitor -
     */

    ArgValue( const std::string& flag, const std::string& name, const std::string& desc,
        Constraint constraint = {},
        const Visitor visitor = {} )
    : ArgBase( flag, name, desc, false, true  ),
        _constraint( constraint ),
        _visitor( visitor ) {}

    /**
     * \param flag - The one character flag that identifies this
     * argument on the command line.
     * \param name - A one word name for the argument.  Can be
     * used as a long flag on the command line.
     * \param desc - A description of what the argument is for or
     * does.
     * \param isRequired - Whether the argument is required on the command
     * line.
     * \param defaultValue - The default value assigned to this argument if it
     * is not present on the command line.
     * \param typeDesc - Description of the type of input
     * \param constraint - A pointer to a Constraint object used
     * to constrain this Arg.
     * \param visitor - An optional visitor.  You probably should not
     * use this unless you have a very good reason.
     */
    ArgValue(
        const std::string& flag, const std::string& name, const std::string& desc,
        const bool isRequired, T defaultValue, std::string typeDesc,
        Constraint constraint = {},
        const Visitor visitor = {} )
    :   ArgBase( flag, name, desc, isRequired, true ),
        _value( defaultValue ),
        _default( defaultValue ),
        _typeDesc(std::move(  typeDesc )),
        _constraint( constraint ),
        _visitor( visitor )
    {}

    /**
     * Handles the processing of the argument.
     * This re-implements the Arg version of this method to set the
     * _value of the argument appropriately.  It knows the difference
     * between labeled and unlabeled.
     * \param i - Pointer the the current argument in the list.
     * \param args - Mutable list of strings. Passed
     * in from main().
     */
    bool processArg( int* i, std::vector< std::string >& args ) override;

    /**
     * Returns the value of the argument.
     */
    const T& getValue() const { return _value; }

    /**
     * Specialization of shortID.
     */
    [[nodiscard]] std::string shortID() const override;

    /**
     * Specialization of longID.
     */
    [[nodiscard]] std::string longID() const override;

    void reset() override;

    /**
     * Prevent accidental copying
     */
    ArgValue( const ArgValue& rhs ) = delete;
    ArgValue& operator=( const ArgValue& rhs ) = delete;
};

/**
 * Implementation of processArg().
 */
template< class T >
bool ArgValue< T >::processArg( int* i, std::vector< std::string >& args ) {
    if( _hasBlanks( args[ *i ] ) ) return false;

    std::string flag = args[ *i ];

    std::string value;
    trimFlag( flag, value );

    if( argMatches( flag ) ) {
        if( _isSet ) {
            throw CmdLineParseException( "Argument already set!", toString() );
        }

        if( delimiter() != ' ' && value.empty() )
            throw ArgParseException(
                "Couldn't find delimiter for this argument!", toString() );

        if( value.empty() ) {
            (*i)++;
            if( static_cast< unsigned int >(*i) < args.size() )
                _extractValue( args[ *i ] );
            else
                throw ArgParseException( "Missing a value for this argument!",
                                         toString() );
        }
        else {
            _extractValue( value );
        }

        _isSet = true;
        _setBy = flag;
        _visit();
        return true;
    }
    return false;
}

/**
 * Implementation of shortID.
 */
template< class T >
std::string ArgValue< T >::shortID() const {
    return fmt::format("{} <{}>", ArgBase::shortID(), _typeDesc );
}

/**
 * Implementation of longID.
 */
template< class T >
std::string ArgValue< T >::longID() const {
    return fmt::format("{} <{}>", ArgBase::longID(), _typeDesc );
}

template< class T >
void ArgValue< T >::reset() {
    ArgBase::reset();
    _value = _default;
}
} // namespace TCLAP

#endif  // TCLAP_VALUE_ARG_H
