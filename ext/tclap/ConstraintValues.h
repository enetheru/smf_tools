/******************************************************************************
 *
 *  file:  ValuesConstraint.h
 *
 *  Copyright (c) 2005, Michael E. Smoot
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

#ifndef TCLAP_VALUES_CONSTRAINT_H
#define TCLAP_VALUES_CONSTRAINT_H

#include <tclap/ConstraintBase.h>

#include <string>
#include <vector>

#include <format>

namespace TCLAP {

/**
 * A Constraint that constrains the Arg to only those values specified
 * in the constraint.
 */
template <class T>
class ValuesConstraint final : public ConstraintBase<T> {
public:
    /**
     * Constructor.
     * \param allowed - vector of allowed values.
     */
    explicit ValuesConstraint(std::vector<T> &allowed);

    /**
     * Virtual destructor.
     */
    virtual ~ValuesConstraint() {}

    /**
     * Returns a description of the Constraint.
     */
    virtual std::string description() const;

    /**
     * Returns the short ID for the Constraint.
     */
    virtual std::string shortID() const;

    /**
     * The method used to verify that the value parsed from the command
     * line meets the constraint.
     * \param value - The value that will be checked.
     */
    virtual bool check(const T &value) const;

protected:
    /**
     * The list of valid values.
     */
    std::vector<T> _allowed;

    /**
     * The string used to describe the allowed values of this constraint.
     */
    std::string _typeDesc;
};

template <class T>
ValuesConstraint<T>::ValuesConstraint(std::vector<T> &allowed)
    : _allowed(allowed),_typeDesc() {
    std::string delim;
    for( const auto &v : allowed ) {
        _typeDesc += std::exchange(delim, "|") += std::format("{}", v );
    }
}

template <class T>
bool ValuesConstraint<T>::check(const T &value) const {
    if (std::find(_allowed.begin(), _allowed.end(), value) == _allowed.end())
        return false;
    return true;
}

template <class T>
std::string ValuesConstraint<T>::shortID() const {
    return _typeDesc;
}

template <class T>
std::string ValuesConstraint<T>::description() const {
    return _typeDesc;
}

}  // namespace TCLAP
#endif  // TCLAP_VALUES_CONSTRAINT_H
