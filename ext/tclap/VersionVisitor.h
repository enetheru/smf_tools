/******************************************************************************
 *
 *  file:  VersionVisitor.h
 *
 *  Copyright (c) 2003, Michael E. Smoot .
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

#ifndef TCLAP_VERSION_VISITOR_H
#define TCLAP_VERSION_VISITOR_H

#include <tclap/CmdLineInterface.h>
#include <tclap/CmdLineOutput.h>
#include <tclap/Visitor.h>

namespace TCLAP {
/**
 * A Visitor that will call the version method of the given CmdLineOutput
 * for the specified CmdLine object and then exit.
 */
class VersionVisitor final : public Visitor {
public:
    /**
     * Prevent accidental copying
     */
    VersionVisitor( const VersionVisitor& rhs ) = delete;
    VersionVisitor& operator=( const VersionVisitor& rhs ) = delete;

protected:
    /**
     * The CmdLine of interest.
     */
    CmdLineInterface* _cmd;

    /**
     * The output object.
     */
    CmdLineOutput** _out;

public:
    /**
     * Constructor.
     * \param cmd - The CmdLine the output is generated for.
     * \param out - The type of output.
     */
    VersionVisitor( CmdLineInterface* cmd, CmdLineOutput** out )
        : _cmd( cmd ), _out( out ) {}

    /**
     * Calls the version method of the output object using the
     * specified CmdLine.
     */
    void visit() override {
        (*_out)->version( *_cmd );
        throw ExitException( 0 );
    }
};
} // namespace TCLAP

#endif  // TCLAP_VERSION_VISITOR_H
