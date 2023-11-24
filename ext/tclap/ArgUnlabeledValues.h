/******************************************************************************
 *
 *  file:  UnlabeledMultiArg.h
 *
 *  Copyright (c) 2003, Michael E. Smoot.
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

#ifndef TCLAP_UNLABELED_MULTI_ARG_H
#define TCLAP_UNLABELED_MULTI_ARG_H

#include <tclap/ArgValueMulti.h>

#include <list>
#include <string>
#include <vector>

namespace TCLAP {

/**
 * Just like a MultiArg, except that the arguments are unlabeled.  Basically,
 * this Arg will slurp up everything that hasn't been matched to another
 * Arg.
 */
template <class T>
class ArgUnlabeledValues final : public ArgValueMulti<T> {
    // If compiler has two stage name lookup (as gcc >= 3.4 does)
    // this is required to prevent undef. symbols
    //FIXME using ArgValueMulti<T>::_ignoreable;
    using ArgValueMulti<T>::_hasBlanks;
    using ArgValueMulti<T>::_extractValue;
    using ArgValueMulti<T>::_typeDesc;
    using ArgValueMulti<T>::_name;
    using ArgValueMulti<T>::_description;
    using ArgValueMulti<T>::_isSet;
    using ArgValueMulti<T>::_setBy;
    using ArgValueMulti<T>::toString;

    using Visitor = std::function<void(const ArgUnlabeledValues&)>;
    using Constraint = std::shared_ptr<ConstraintBase< T >>;


    Constraint _constraint;
    Visitor _visitor;

public:
    /**
     * Constructor.
     * \param name - The name of the Arg. Note that this is used for
     * identification, not as a long flag.
     * \param desc - A description of what the argument is for or
     * does.
     * \param constraint
     * \param visitor - An optional visitor.  You probably should not
     * use this unless you have a very good reason.
     */
    ArgUnlabeledValues(const std::string &name, const std::string &desc,
                      Constraint constraint = {}, Visitor visitor = {} )
    : ArgValueMulti<T>( {}, name, desc ),
        _constraint( constraint ), _visitor( visitor ) {}

    /**
     * Constructor.
     * \param name - The name of the Arg. Note that this is used for
     * identification, not as a long flag.
     * \param desc - A description of what the argument is for or
     * does.
     * \param req - Whether the argument is required on the command
     *  line.
     * \param defaultValue
     * \param typeDesc - A short, human readable description of the
     * type that this object expects.  This is used in the generation
     * of the USAGE statement.  The goal is to be helpful to the end user
     * of the program.
     * \param constraint
     * \param visitor - An optional visitor.  You probably should not
     * use this unless you have a very good reason.
     */
    ArgUnlabeledValues(const std::string &name, const std::string &desc,
                      bool req, const T &defaultValue, const std::string &typeDesc,
                      ConstraintBase<T> constraint= {}, Visitor visitor = {})
    : ArgValueMulti<T>( {}, name, desc, req, defaultValue, typeDesc ),
        _constraint( constraint ), _visitor( visitor ) {}

    /**
     * Handles the processing of the argument.
     * This re-implements the Arg version of this method to set the
     * _value of the argument appropriately.  It knows the difference
     * between labeled and unlabeled.
     * \param i - Pointer the the current argument in the list.
     * \param args - Mutable list of strings. Passed from main().
     */
    virtual bool processArg(int *i, std::vector<std::string> &args);

    /**
     * Operator ==.
     * \param a - The Arg to be compared to this.
     */
    virtual bool operator==(const ArgBase &a) const;

    /**
     * Pushes this to back of list rather than front.
     * \param argList - The list this should be added to.
     */
    void addToList(std::list<ArgBase *> &argList) const;

    [[nodiscard]] virtual bool hasLabel() const { return false; }
};

template <class T>
bool ArgUnlabeledValues<T>::processArg(int *i, std::vector<std::string> &args) {
    if (_hasBlanks(args[*i])) return false;

    // never ignore an unlabeled multi arg

    // always take the first value, regardless of the start string
    try {
        _extractValue(args[(*i)]);
    }
    catch (std::invalid_argument const& ex)
    {
        SPDLOG_ERROR( "std::invalid_argument::what(): {}", ex.what() );
        return false;
    }
    catch (std::out_of_range const& ex)
    {
        SPDLOG_ERROR( "std::out_of_range::what(): {}", ex.what() );
        return false;
    }

    /*
    // continue taking args until we hit the end or a start string
    while ( (unsigned int)(*i)+1 < args.size() &&
            args[(*i)+1].find_first_of( Arg::flagStartString() ) != 0 &&
            args[(*i)+1].find_first_of( Arg::nameStartString() ) != 0 )
        _extractValue( args[++(*i)] );
    */

    _isSet = true;
    _setBy = args[*i];

    return true;
}

template <class T>
bool ArgUnlabeledValues<T>::operator==(const ArgBase &a) const {
    if (_name == a.getName() || _description == a.getDescription())
        return true;
    return false;
}

template <class T>
void ArgUnlabeledValues<T>::addToList(std::list<ArgBase *> &argList) const {
    argList.push_back(const_cast<ArgBase *>(static_cast<const ArgBase *const>(this)));
}
}  // namespace TCLAP

#endif  // TCLAP_UNLABELED_MULTI_ARG_H
