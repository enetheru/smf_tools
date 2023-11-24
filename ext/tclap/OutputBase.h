/******************************************************************************
 *
 *  file:  CmdLineOutput.h
 *
 *  Copyright (c) 2004, Michael E. Smoot
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

#ifndef TCLAP_CMD_LINE_OUTPUT_H
#define TCLAP_CMD_LINE_OUTPUT_H

#include <tclap/ArgBase.h>
#include <tclap/GroupBase.h>

#include <list>

namespace TCLAP {
class CmdLineInterface;
class Exception;

/**
 * The interface that any output object must implement.
 */
class CmdLineOutput {
public:
    /**
     * Virtual destructor.
     */
    virtual ~CmdLineOutput() = default;

    /**
     * Generates some sort of output for the USAGE.
     * \param c - The CmdLine object the output is generated for.
     */
    virtual void usage( CmdLineInterface& c ) = 0;

    /**
     * Generates some sort of output for the version.
     * \param c - The CmdLine object the output is generated for.
     */
    virtual void version( CmdLineInterface& c ) = 0;

    /**
     * Generates some sort of output for a failure.
     * \param c - The CmdLine object the output is generated for.
     * \param e - The ArgException that caused the failure.
     */
    virtual void failure( CmdLineInterface& c, Exception& e ) = 0;
};

inline bool isInArgGroup( const std::shared_ptr<ArgBase> arg, const std::list< Group* >& argSets ) {
    for( const auto group : argSets ) {
        if( std::ranges::find( *group, arg ) != group->end() ) {
            return true;
        }
    }
    return false;
}

inline void removeArgsInArgGroups( std::list< std::shared_ptr<ArgBase> >& argList, const std::list< Group* >& argSets ) {
    argList.remove_if( [&argSets]( const auto arg ) { return isInArgGroup( arg, argSets ); } );
}
} // namespace TCLAP

#endif  // TCLAP_CMD_LINE_OUTPUT_H
