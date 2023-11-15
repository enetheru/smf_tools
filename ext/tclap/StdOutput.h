// -*- Mode: c++; c-basic-offset: 4; tab-width: 4; -*-

/******************************************************************************
 *
 *  file:  StdOutput.h
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

#ifndef TCLAP_STD_OUTPUT_H
#define TCLAP_STD_OUTPUT_H

#include <tclap/Arg.h>
#include <tclap/ArgGroup.h>
#include <tclap/CmdLineInterface.h>
#include <tclap/CmdLineOutput.h>

#include <algorithm>
#include <cctype>
#include <list>
#include <ranges>
#include <string>
#include <utility>
#include <vector>
#include <fmt/core.h>

namespace TCLAP {

/**
 * A class that isolates any output from the CmdLine object so that it
 * may be easily modified.
 */
class StdOutput : public CmdLineOutput {
public:
    /**
     * Prints the usage to stdout.  Can be overridden to
     * produce alternative behavior.
     * \param cmd - The CmdLine object the output is generated for.
     */
    virtual void usage(CmdLineInterface &cmd);

    /**
     * Prints the version to stdout. Can be overridden
     * to produce alternative behavior.
     * \param cmd - The CmdLine object the output is generated for.
     */
    virtual void version(CmdLineInterface &cmd);

    /**
     * Prints (to stderr) an error message, short usage
     * Can be overridden to produce alternative behavior.
     * \param cmd - The CmdLine object the output is generated for.
     * \param e - The ArgException that caused the failure.
     */
    virtual void failure(CmdLineInterface &cmd, ArgException &e);

protected:
    /**
     * Writes a brief usage message with short args.
     * \param cmd - The CmdLine object the output is generated for.
     */
    static void _shortUsage(CmdLineInterface &cmd);

    /**
     * Writes a longer usage message with long and short args,
     * provides descriptions and prints message.
     * \param cmd - The CmdLine object the output is generated for.
     */
    static void _longUsage(CmdLineInterface &cmd);

    /**
     * This function inserts line breaks and indents long strings
     * according the  params input. It will only break lines at spaces,
     * commas and pipes.
     * \param s - The string to be printed.
     * \param maxWidth - The maxWidth allowed for the output line.
     * \param indentSpaces - The number of spaces to indent the first line.
     * \param secondLineOffset - The number of spaces to indent the second
     * and all subsequent lines in addition to indentSpaces.
     */
    static void spacePrint(const std::string &s, int maxWidth,
                           int indentSpaces, int secondLineOffset);
};

inline void StdOutput::version(CmdLineInterface &cmd) {
    fmt::println("\n{}  version: {}\n", cmd.getProgramName(), cmd.getVersion() );
}

inline void StdOutput::usage(CmdLineInterface &cmd) {
    fmt::println("\nUSAGE: \n");
    _shortUsage(cmd);
    fmt::println("\n\nWhere: \n");
    _longUsage(cmd);
    fmt::println("");
}

inline void StdOutput::failure(CmdLineInterface &cmd, ArgException &e) {

    fmt::println("PARSE ERROR: {}", e.argId() );
    fmt::println("             {}", e.error() );
    fmt::println("");

    if (cmd.hasHelpAndVersion()) {
        fmt::println( "Brief USAGE: " );

        _shortUsage(cmd);

        fmt::println("\nFor complete USAGE and HELP type: " );
        fmt::println("   {} {}help\n", cmd.getProgramName(), Arg::nameStartString() );

    } else {
        usage(cmd);
    }

    throw ExitException(1);
}

inline bool cmpSwitch(const char &a, const char &b) {
    const int lowa = std::tolower(a);
    const int lowb = std::tolower(b);

    if (lowa == lowb) {
        return a < b;
    }

    return lowa < lowb;
}

inline bool cmpArgSwitch(const Arg *a, const Arg *b) {
    const int lowa = std::tolower( *a->getFlag().c_str() );
    const int lowb = std::tolower( *b->getFlag().c_str() );

    if (lowa == lowb) {
        return a < b;
    }

    return lowa < lowb;
}

namespace internal {
inline bool IsVisibleShortSwitch(const Arg *arg) {
    return !(arg->getName() == Arg::ignoreNameString()
            || arg->isValueRequired()
            || arg->getFlag().empty() )
        && arg->visibleInHelp();
}

inline bool IsVisibleLongSwitch(const Arg *arg) {
    return arg->getName() != Arg::ignoreNameString()
        && !arg->isValueRequired()
        && arg->getFlag().empty()
        && arg->visibleInHelp();
}

inline bool IsVisibleOption(const Arg *arg) {
    return arg->getName() != Arg::ignoreNameString()
        && arg->isValueRequired()
        && arg->hasLabel()
        && arg->visibleInHelp();
}

inline bool CompareShortID(const Arg *a, const Arg *b) {
    if (a->getFlag().empty() && ! b->getFlag().empty() ) {
        return false;
    }
    if (b->getFlag().empty() && !a->getFlag().empty() ) {
        return true;
    }

    return a->shortID() < b->shortID();
}

// TODO: Fix me not to put --gopt before -f
inline auto CompareOptions(const std::pair<const Arg *, bool>&a, const std::pair<const Arg *, bool>&b) -> bool {
    // First optional, then required
    if (!a.second && b.second) {
        return true;
    }
    if (a.second && !b.second) {
        return false;
    }

    return CompareShortID(a.first, b.first);
}
}  // namespace internal

/**
 * Usage statements should look like the manual pages.  Options w/o
 * operands come first, in alphabetical order inside a single set of
 * braces, upper case before lower case (AaBbCc...).  Next are options
 * with operands, in the same order, each in braces.  Then required
 * arguments in the order they are specified, followed by optional
 * arguments in the order they are specified.  A bar (`|') separates
 * either/or options/arguments, and multiple options/arguments which
 * are specified together are placed in a single set of braces.
 *
 * Use getprogname() instead of hardcoding the program name.
 *
 * "usage: f [-aDde] [-b b_arg] [-m m_arg] req1 req2 [opt1 [opt2]]\n"
 * "usage: f [-a | -b] [-c [-de] [-n number]]\n"
 */
inline void StdOutput::_shortUsage( CmdLineInterface &cmd )
{
    std::ostringstream outp;
    outp << cmd.getProgramName() + " ";

    std::string switches = Arg::flagStartString();

    auto exclusiveGroups = std::ranges::views::filter( cmd.getArgGroups(), [](const auto group ){
        return group->isExclusive()
            && CountVisibleArgs(*group) > 1;
    } );
    auto nonExclusiveGroups = std::ranges::views::filter( cmd.getArgGroups(), [](const auto group ){
        return (!group->isExclusive() && CountVisibleArgs(*group) > 0 )
            || ( group->isExclusive() && CountVisibleArgs(*group) == 1 );
    } );

    auto nonExclusiveArgs = std::ranges::views::filter( cmd.getArgGroups(), [](const auto group ){
        return (!group->isExclusive() && CountVisibleArgs(*group) > 0 )
            || ( group->isExclusive() && CountVisibleArgs(*group) == 1 );
    } ) | std::ranges::views::transform( [](const auto group_p ){
        return std::ranges::ref_view( *group_p );
    } ) | std::views::join;

    auto nonExclusiveArgsView = std::ranges::ref_view( nonExclusiveArgs );
    // "exclusive groups" that have at most a single item dont make sense to include in the exclusive group list
    // This can happen if args are hidden in help for example.

    // First short switches (needs to be special because they are all stuck together).
    for( const auto arg : nonExclusiveArgsView | std::views::filter(internal::IsVisibleShortSwitch ) ) {
        switches += arg->getFlag();
    }
    std::ranges::sort(switches, cmpSwitch);

    outp << " [" << switches << ']';

    // Now do long switches (e.g., --version, but no -v)
    std::vector<Arg *> longSwitches;
    for( const auto arg : nonExclusiveArgsView | std::views::filter(internal::IsVisibleLongSwitch ) ) {
        longSwitches.push_back( arg );
    }
    std::ranges::sort(longSwitches, internal::CompareShortID);

    for( const auto arg : longSwitches ) {
        outp << " [" << arg->shortID() << ']';
    }

    // Now do all exclusive groups
    for( const auto argGroup : exclusiveGroups ){
        outp << (argGroup->isRequired() ? " {" : " [");

        std::vector<Arg *> args;
        for(const auto arg : *argGroup ){
            if( arg->visibleInHelp() ){
                args.push_back( arg );
            }
        }
        std::ranges::sort(args, internal::CompareShortID);

        std::string sep = "";
        for( const auto arg : args ) {
            outp << sep << arg->shortID();
            sep = "|";
        }

        outp << (argGroup->isRequired() ? '}' : ']');
    }

    // Next do options, we sort them later by optional first.
    std::vector<std::pair<const Arg *, bool> > options;
    for( const auto argGroup : nonExclusiveGroups ){
        for( const auto arg : *argGroup ) {
            //foreach argument in the non exclusive groups

            int visible = CountVisibleArgs(*argGroup);          // How many visible arguments are there in the group?
            bool required = arg->isRequired();                  // Is the argument required?
            if (internal::IsVisibleOption(arg)) {               // If the arg is visible
                if( visible == 1 && argGroup->isRequired() ){   // If the arg is singular in its visibility, and the group is required
                    required = true;
                }
                options.push_back({ arg, required } );
            }
        }
    }
    std::ranges::sort(options, internal::CompareOptions );

    for( const auto &[arg, required] : options ) {
        outp << (required ? " " : " [");
        outp << arg->shortID();
        outp << (required ? "" : "]");
    }

    // Next do arguments ("unlabled") in order of definition
    for( const auto arg : nonExclusiveArgsView
        | std::views::filter( [](const auto a ) {
            return a->getName() != Arg::ignoreNameString()
                && a->isValueRequired()
                && !a->hasLabel()
                && a->visibleInHelp(); } ) )
    {
        outp << (arg->isRequired() ? " " : " [");
        outp << arg->shortID();
        outp << (arg->isRequired() ? "" : "]");
    }

    // if the program name is too long, then adjust the second line offset
    int secondLineOffset = static_cast<int>(cmd.getProgramName().length()) + 2;
    if (secondLineOffset > 75 / 2) secondLineOffset = 75 / 2;

    spacePrint( outp.str(), 75, 3, secondLineOffset);
}

inline void StdOutput::_longUsage(CmdLineInterface &cmd) {
    const std::string message = cmd.getMessage();
    const std::list<ArgGroup *> argSets = cmd.getArgGroups();

    std::list<Arg *> unlabled;
    for( const auto argGroup : argSets ) {
        const int  visible       = CountVisibleArgs( *argGroup );
        const bool exclusive     = visible > 1 && argGroup->isExclusive();
        const bool forceRequired = visible == 1 && argGroup->isRequired();
        if (exclusive) {
            spacePrint( argGroup->isRequired() ? "One of:" : "Either of:", 75, 3, 0);
        }

        for( const auto arg : *argGroup ) {
            if (!arg->visibleInHelp()) {
                continue;
            }

            if (!arg->hasLabel()) {
                unlabled.push_back( arg );
                continue;
            }

            const bool required = arg->isRequired() || forceRequired;
            if (exclusive) {
                spacePrint( arg->longID(), 75, 6, 3);
                spacePrint( arg->getDescription(required), 75, 8, 0);
            } else {
                spacePrint( arg->longID(), 75, 3, 3);
                spacePrint( arg->getDescription(required), 75, 5, 0);
            }
            fmt::print("\n");
        }
    }

    for( const auto arg : unlabled ){
        spacePrint( arg->longID(), 75, 3, 3);
        spacePrint( arg->getDescription(), 75, 5, 0);
        fmt::print("\n");
    }

    if (!message.empty()) {
        spacePrint( message, 75, 3, 0);
    }
}

namespace {
void fmtPrintLine( const std::string &s, const int maxWidth,
    const int indentSpaces, int secondLineOffset)
{
    const std::string splitChars(" ,|");
    int               maxChars = maxWidth - indentSpaces;
    std::string       indentString(indentSpaces, ' ');
    int               from = 0;
    int               to   = 0;
    const int         end  = s.length();

    for (;;) {
        if (end - from <= maxChars) {
            // Rest of string fits on line, just print the remainder
            fmt::print("{}{}\n",indentString, s.substr(from));
            return;
        }

        // Find the next place where it is good to break the string
        // (to) by finding the place where it is too late (tooFar) and
        // taking the previous one.
        int tooFar = to;
        while (tooFar - from <= maxChars &&
               static_cast<std::size_t>(tooFar) != std::string::npos) {
            to = tooFar;
            tooFar = s.find_first_of(splitChars, to + 1);
        }

        if (to == from) {
            // In case there was no good place to break the string,
            // just break it in the middle of a word at line length.
            to = from + maxChars - 1;
        }

        if (s[to] != ' ') {
            // Include delimiter before line break, unless it's a space
            to++;
        }

        fmt::print("{}{}\n", indentString,s.substr(from, to - from) );

        // Avoid printing extra white space at start of a line
        for (; s[to] == ' '; to++) {
        }
        from = to;

        if (secondLineOffset != 0) {
            // Adjust offset for following lines
            indentString.insert(indentString.end(), secondLineOffset, ' ');
            maxChars -= secondLineOffset;
            secondLineOffset = 0;
        }
    }
}
}  // namespace

inline void StdOutput::spacePrint( const std::string&s,
    const int maxWidth, int indentSpaces, const int secondLineOffset)
{
    std::stringstream ss(s);
    std::string line;
    std::getline(ss, line);
    fmtPrintLine( line, maxWidth, indentSpaces, secondLineOffset);
    indentSpaces += secondLineOffset;

    while (std::getline(ss, line)) {
        fmtPrintLine( line, maxWidth, indentSpaces, 0);
    }
}

}  // namespace TCLAP

#endif  // TCLAP_STD_OUTPUT_H
