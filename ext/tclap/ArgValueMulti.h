/******************************************************************************
 *
 *  file:  MultiArg.h
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

#ifndef TCLAP_MULTI_ARG_H
#define TCLAP_MULTI_ARG_H

#include <tclap/ArgBase.h>
#include <tclap/ArgValue.h>
#include <tclap/ConstraintBase.h>

#include <string>
#include <vector>

namespace TCLAP {
/**
 * An argument that allows multiple values of type T to be specified.  Very
 * similar to a ValueArg, except a vector of values will be returned
 * instead of just one.
 */
template <class T>
class ArgValueMulti : public ArgValue<T>, protected std::vector<T> {
public:
    using std::vector<T>::begin;
    using std::vector<T>::end;
    using std::vector<T>::iterator;
    using std::vector<T>::const_iterator;

    using Visitor = std::function<void(const ArgValueMulti&)>;
    using Constraint = std::shared_ptr<ConstraintBase<T>>;

protected:

    /**
     * Used to support the reset() method so that ValueArg can be
     * reset to their constructed value.
     */
    T _default;

    /**
     * The description of type T to be used in the usage.
     */
    std::string _typeDesc = type_name<T>();

    /**
     * A list of constraint on this Arg.
     */
    Constraint _constraint;

    /**
     * std::function visitor, is called after validation succeeds.
     */
    Visitor _visitor{};

    /**
     * Used by MultiArg to decide whether to keep parsing for this
     * arg.
     */
    bool _allowMore{true};

    /**
     * Performs the special handling described by the Visitor.
     */
    void _visit() const override { if(_visitor)_visitor( *this ); }

    /**
     * Extracts the value from the string.
     * Attempts to parse string as type T, if this fails an exception
     * is thrown.
     * \param val - The string to be read.
     */
    void _extractValue( const std::string& val ) {
        this->push_back( ExtractValue<T>( val ) );
        if( _constraint ){
            auto [passed, msg] = _constraint->check( *this );
            if( passed != SUCCESS ) throw CmdLineParseException( msg, ArgValue<T>::toString() );
        }
    }
public:
    /**
     * Constructor.
     * \param flag - The one character flag that identifies this
     * argument on the command line.
     * \param name - A one word name for the argument.  Can be
     * used as a long flag on the command line.
     * \param desc - A description of what the argument is for or
     * does.
     * \param constraint
     * \param visitor - An optional visitor.  You probably should not
     * use this unless you have a very good reason.
     */
    ArgValueMulti( const std::string& flag, const std::string& name, const std::string& desc,
        Constraint constraint = {}, Visitor visitor = {} )
    : ArgValue<T>( flag, name, desc ),
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
    ArgValueMulti(
        const std::string& flag, const std::string& name, const std::string& desc,
        const bool isRequired, T defaultValue, std::string typeDesc,
        Constraint constraint = {}, Visitor visitor = {} )
    :   ArgValue<T>( flag, name, desc, isRequired, true, defaultValue, typeDesc ),
        _constraint( constraint ),
        _visitor( visitor ) {
        this->push_back( defaultValue );
    }

    /**
     * Handles the processing of the argument.
     * This re-implements the Arg version of this method to set the
     * _value of the argument appropriately.  It knows the difference
     * between labeled and unlabeled.
     * \param i - Pointer the the current argument in the list.
     * \param args - Mutable list of strings. Passed from main().
     */
    bool processArg(int *i, std::vector<std::string> &args) override;

    /**
     * Returns the a short id string.  Used in the usage.
     */
    [[nodiscard]] std::string shortID() const override;

    /**
     * Returns the a long id string.  Used in the usage.
     */
    [[nodiscard]] std::string longID() const override;

    virtual bool allowMore();

    void reset() override;

    /**
     * Prevent accidental copying
     */
    ArgValueMulti(const ArgValueMulti &rhs)             = delete;
    ArgValueMulti & operator=(const ArgValueMulti &rhs) = delete;
};


template <class T>
bool ArgValueMulti<T>::processArg(int *i, std::vector<std::string> &args) {
    if (ArgBase::_hasBlanks(args[*i])) return false;

    std::string flag = args[*i];
    std::string value;

    ArgBase::trimFlag(flag, value);

    if (ArgBase::argMatches(flag)) {
        if (ArgBase::delimiter() != ' ' && value.empty())
            throw ArgParseException(
                "Couldn't find delimiter for this argument!", ArgBase::toString());

        // always take the first one, regardless of start string
        if (value.empty()) {
            (*i)++;
            if (static_cast<unsigned int>(*i) < args.size())
                _extractValue(args[*i]);
            else
                throw ArgParseException("Missing a value for this argument!",
                                       ArgBase::toString());
        } else {
            _extractValue(value);
        }

        ArgBase::_isSet = true;
        ArgBase::_setBy = flag;
        _visit();

        return true;
    }
    return false;
}

/**
 *
 */
template <class T>
std::string ArgValueMulti<T>::shortID() const {
    return fmt::format("{} <{}>", ArgBase::shortID(), _typeDesc );
}

/**
 *
 */
template <class T>
std::string ArgValueMulti<T>::longID() const {
    return fmt::format("{} <{}>", ArgBase::longID(), _typeDesc );
}

template <class T>
bool ArgValueMulti<T>::allowMore() {
    const bool am = _allowMore;
    _allowMore = true;
    return am;
}

template <class T>
void ArgValueMulti<T>::reset() {
    ArgBase::reset();
    this->clear();
}

}  // namespace TCLAP

#endif  // TCLAP_MULTI_ARG_H
