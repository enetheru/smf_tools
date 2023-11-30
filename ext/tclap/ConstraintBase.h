/******************************************************************************
 *
 *  file:  Constraint.h
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

#ifndef TCLAP_CONSTRAINT_H
#define TCLAP_CONSTRAINT_H

#include <string>

namespace TCLAP {

template<class T>
class ArgValue;

/**
 * The interface that defines the interaction between the Arg and Constraint.
 */

enum CheckResult {
 HARD_FAILURE,
 SOFT_FAILURE,
 SUCCESS
};


template< class T >
class ConstraintBase {
public:
    using RetVal = std::pair<CheckResult, std::string>; //TODO make this a struct so we can get named values in hints
    /**
     * Returns a description of the Constraint.
     */
    [[nodiscard]] virtual std::string description() const = 0;

    /**
     * The method used to verify that the value parsed from the command
     * line meets the constraint.
     * \param arg - The value that will be checked.
     * @retval - A pair of { result, message }
     */
    virtual RetVal check( const ArgValue<T>& arg ) const = 0;

    /**
     * Destructor.
     * Silences warnings about Constraint being a base class with virtual
     * functions but without a virtual destructor.
     */
    virtual ~ConstraintBase() = default;
};
} // namespace TCLAP

#endif  // TCLAP_CONSTRAINT_H