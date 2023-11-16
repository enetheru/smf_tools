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

#ifndef TCLAP_ARG_GROUP_H
#define TCLAP_ARG_GROUP_H

#include <tclap/Arg.h>
#include <tclap/ArgContainer.h>
#include <tclap/CmdLineInterface.h>

#include <string>
#include <utility>

namespace TCLAP {
/**
 * ArgGroup is the base class for implementing groups of arguments
 * that are mutually exclusive (it replaces the deprecated xor
 * handler). It is not expected to be used directly, rather one of the
 * EitherOf or OneOf derived classes are used.
 */
class ArgGroup : public ArgContainer, protected std::vector< std::shared_ptr<Arg> > {
public:
    using std::vector< std::shared_ptr<Arg> >::begin;
    using std::vector< std::shared_ptr<Arg> >::end;
    using std::vector< std::shared_ptr<Arg> >::iterator;
    using std::vector< std::shared_ptr<Arg> >::const_iterator;

    ~ArgGroup() override = default;

    /// Add an argument to this arg group
    ArgContainer& add( std::shared_ptr<Arg> arg ) override;

    template< class ArgType, typename ...args >
    std::shared_ptr<ArgType> create( const args & ... fwd ) {
        auto item = std::make_shared<ArgType>( fwd ... );
        add( item );
        return item;
    }

    /**
     * Validates that the constraints of the ArgGroup are satisfied.
     *
     * @internal
     * Throws an CmdLineParseException if there is an issue (except
     * missing required argument, in which case true is returned).
     *
     * @retval true iff a required argument was missing.
     */
    virtual bool validate() = 0;

    /**
     * Returns true if this argument group is required
     *
     * @internal
     */
    [[nodiscard]] virtual bool isRequired() const = 0;

    /**
     * Returns true if this argument group is exclusive.
     *
     * @internal
     * Being exclusive means there is a constraint so that some
     * arguments cannot be selected at the same time.
     */
    [[nodiscard]] virtual bool isExclusive() const = 0;

    /**
     * Used by the parser to connect itself to this arg group.
     *
     * @internal
     * This is needed so that existing and subsequently added args (in
     * this arg group) are also added to the parser (and checked for
     * consistency with other args).
     */
    void setParser( CmdLineInterface& parser ) {
        if( _parser ) {
            throw SpecificationException( "Arg group can have only one parser" );
        }

        _parser = &parser;
        for( const auto &arg : *this ) {
            parser.addToArgList( arg );
        }
    }

    /**
     * If arguments in this group should show up as grouped in help.
     */
    [[nodiscard]] bool showAsGroup() const { return _showAsGroup; }

    /// Returns the argument group's name.
    [[nodiscard]] virtual std::string getName() const;


protected:
    // No direct instantiation
    explicit ArgGroup( std::string name = {} ) : vector(), _name(std::move(name)) { }

public:
    ArgGroup( const ArgGroup& ) = delete;
    ArgGroup& operator=( const ArgGroup& ) = delete; // no copy

protected:
    CmdLineInterface* _parser {};
    std::string _name;
    bool _showAsGroup;
};

/**
 * Implements common functionality for exclusive argument groups.
 *
 * @internal
 */
class ExclusiveArgGroup : public ArgGroup {
public:
    inline bool validate() override;
    [[nodiscard]] bool isExclusive() const override { return true; }

    ArgContainer& add( const std::shared_ptr<Arg> arg ) override {
        if( arg->isRequired() ) {
            throw SpecificationException(
                "Required arguments are not allowed in an exclusive grouping.",
                arg->longID() );
        }

        return ArgGroup::add( arg );
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
class AnyOf : public ArgGroup {
public:
    AnyOf() = default;

    bool validate() override {
        return false; /* All good */
    }

    [[nodiscard]] bool isExclusive() const override { return false; }
    [[nodiscard]] bool isRequired() const override { return false; }
};

inline ArgContainer& ArgGroup::add( std::shared_ptr<Arg> arg ) {
    auto find_func = [&arg]( const std::shared_ptr<Arg> &existing ) { return *existing == *arg; };
    if( std::ranges::find_if( *this, find_func ) != this->end() ) {
        throw SpecificationException(
            "Argument with same flag/name already exists!", arg->longID() );
    }

    push_back( arg );
    if( _parser ) {
        _parser->addToArgList( arg );
    }

    return *this;
}

inline bool ExclusiveArgGroup::validate() {
    std::shared_ptr<Arg> existing_arg = nullptr;
    std::string flag;

    for( const auto &arg : *this ) {
        if( arg->isSet() ) {
            if( !existing_arg && existing_arg != arg ) {
                // We found a matching argument, but one was
                // already found previously.
                throw CmdLineParseException(
                    "Only one is allowed.",
                    std::format( "{} AND {} provided.", flag, arg->setBy() ) );
            }

            existing_arg = arg;
            flag = existing_arg->setBy();
        }
    }

    return isRequired() && !existing_arg;
}

inline std::string ArgGroup::getName() const {
    if( !_name.empty() ) return _name;
    std::string list;
    std::string sep; // TODO: this should change for non-exclusive arg groups
    for( const auto &it : *this ) {
        list += std::exchange( sep, " | " ) + it->getName();
    }
    return std::format("{{{}}}", list );
}

/// @internal
inline int CountVisibleArgs( const ArgGroup& g ) {
    int visible = 0;
    for( const auto &it : g ) {
        if( it->visibleInHelp() ) {
            visible++;
        }
    }
    return visible;
}
} // namespace TCLAP

#endif  // TCLAP_ARG_GROUP_H
