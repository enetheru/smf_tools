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

#include <stdexcept>
#include <string>

namespace TCLAP {

template<class T>
class ValueArg;

/**
 * The interface that defines the interaction between the Arg and Constraint.
 */
template< class T >
class Constraint {
public:
    enum CheckResult {
        HARD_FAILURE,
        SOFT_FAILURE,
        SUCCESS
    };
    using RetVal = std::pair<CheckResult, std::string>;
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
    virtual RetVal check( const ValueArg<T>& arg ) const = 0;

    /**
     * Destructor.
     * Silences warnings about Constraint being a base class with virtual
     * functions but without a virtual destructor.
     */
    virtual ~Constraint() = default;
};
} // namespace TCLAP

#endif  // TCLAP_CONSTRAINT_H
