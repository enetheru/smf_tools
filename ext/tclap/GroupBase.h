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

#ifndef TCLAP_ARG_GROUPBASE_H
#define TCLAP_ARG_GROUPBASE_H

#include <tclap/ArgBase.h>
#include <tclap/ContainerBase.h>
#include <tclap/CmdLineBase.h>

#include <string>
#include <utility>

namespace TCLAP {
/**
 * ArgGroup is the base class for implementing groups of arguments
 * that are mutually exclusive (it replaces the deprecated xor
 * handler). It is not expected to be used directly, rather one of the
 * EitherOf or OneOf derived classes are used.
 */
class Group : public Container, protected std::vector< std::shared_ptr<ArgBase> > {
public:
    using std::vector< std::shared_ptr<ArgBase> >::begin;
    using std::vector< std::shared_ptr<ArgBase> >::end;
    using std::vector< std::shared_ptr<ArgBase> >::iterator;
    using std::vector< std::shared_ptr<ArgBase> >::const_iterator;

    ~Group() override = default;

    /// Add an argument to this arg group
    Container& add( std::shared_ptr<ArgBase> arg ) override;

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
    explicit Group( std::string name = {} ) : vector(), _name(std::move(name)) { }

public:
    Group( const Group& ) = delete;
    Group& operator=( const Group& ) = delete; // no copy

protected:
    CmdLineInterface* _parser {};
    std::string _name;
    bool _showAsGroup{};
};

inline Container& Group::add( std::shared_ptr<ArgBase> arg ) {
    auto find_func = [&arg]( const std::shared_ptr<ArgBase> &existing ) { return *existing == *arg; };
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

inline std::string Group::getName() const {
    if( !_name.empty() ) return _name;
    std::string list;
    std::string sep; // TODO: this should change for non-exclusive arg groups
    for( const auto &it : *this ) {
        list += std::exchange( sep, " | " ) + it->getName();
    }
    return fmt::format("{{{}}}", list );
}

/// @internal
inline int CountVisibleArgs( const Group& g ) {
    int visible = 0;
    for( const auto &it : g ) {
        if( it->isVisible() ) {
            visible++;
        }
    }
    return visible;
}
} // namespace TCLAP

#endif  // TCLAP_ARG_GROUPBASE_H
