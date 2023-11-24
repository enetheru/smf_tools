/******************************************************************************
 *
 *  file:  ArgGroup.h
 *
 *  Copyright (c) 2017 Google LLC.
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

#ifndef TCLAP_ARG_GROUPS_H
#define TCLAP_ARG_GROUPS_H

#include <tclap/GroupBase.h>

#include <string>
#include <utility>

namespace TCLAP {

/**
 * Implements common functionality for exclusive argument groups.
 *
 * @internal
 */
class ExclusiveArgGroup : public Group {
public:
    inline bool validate() override;
    [[nodiscard]] bool isExclusive() const override { return true; }

    Container& add( const std::shared_ptr<ArgBase> arg ) override {
        if( arg->isRequired() ) {
            throw SpecificationException(
                "Required arguments are not allowed in an exclusive grouping.",
                arg->longID() );
        }

        return Group::add( arg );
    }

protected:
    ExclusiveArgGroup() = default;
};

/**
 * Implements a group of arguments where at most one can be selected.
 */
class EitherOf final : public ExclusiveArgGroup {
public:
    EitherOf() = default;
    explicit EitherOf( CmdLineInterface& parser ) : ExclusiveArgGroup() {}

    [[nodiscard]] bool isRequired() const override { return false; }
};

/**
 * Implements a group of arguments where exactly one must be
 * selected. This corresponds to the deprecated "xoradd".
 */
class OneOf final : public ExclusiveArgGroup {
public:
    OneOf() = default;
    explicit OneOf( CmdLineInterface& parser ) : ExclusiveArgGroup( ) {}

    [[nodiscard]] bool isRequired() const override { return true; }
};

/**
 * Implements a group of arguments where any combination is possible
 * (including all or none). This is mostly used in case one optional
 * argument allows additional arguments to be specified (for example
 * [-c [-de] [-n <int>]]).
 */
class AnyOf : public Group {
public:
    AnyOf() = default;

    bool validate() override {
        return false; /* All good */
    }

    [[nodiscard]] bool isExclusive() const override { return false; }
    [[nodiscard]] bool isRequired() const override { return false; }
};

inline bool ExclusiveArgGroup::validate() {
    std::shared_ptr<ArgBase> existing_arg = nullptr;
    std::string flag;

    for( const auto &arg : *this ) {
        if( arg->isSet() ) {
            if( !existing_arg && existing_arg != arg ) {
                // We found a matching argument, but one was
                // already found previously.
                throw CmdLineParseException(
                    "Only one is allowed.",
                    fmt::format( "{} AND {} provided.", flag, arg->setBy() ) );
            }

            existing_arg = arg;
            flag = existing_arg->setBy();
        }
    }

    return isRequired() && !existing_arg;
}

} // namespace TCLAP

#endif  // TCLAP_ARG_GROUPS_H
