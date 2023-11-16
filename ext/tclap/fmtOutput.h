#ifndef FMTOUTPUT_H
#define FMTOUTPUT_H

#include <tclap/CmdLineOutput.h>
#include <fmt/core.h>
#include <ranges>


namespace TCLAP {
class fmtOutput final : public CmdLineOutput {
    size_t _maxWidth{ 80 };
    size_t _indentWidth{ 4 };

public:
    fmtOutput() = default;

    explicit fmtOutput( const size_t maxWidth, const size_t indentWidth ) : _maxWidth( maxWidth ),
                                                                            _indentWidth( indentWidth ) {}

    void failure( CmdLineInterface& c, ArgException& e ) override {
        fmt::println( stderr, "TCLAP Error: {}", e.what() );
        fmt::println( stderr, "\t", e.typeDescription() );
        fmt::println( stderr, "\t", e.error() );
        exit( 1 );
    }

    void usage( CmdLineInterface& cmd ) override {
        //Scan options to get Information
        size_t columnWidths[ 3 ]{ 8, 8, 8 };
        std::string groupedShortOpts;

        // Short Opts
        for( const auto& arg : cmd.getArgList() ) {
            if( columnWidths[ 1 ] < arg->getName().length() )columnWidths[ 1 ] = arg->getName().length();
            if( columnWidths[ 2 ] < arg->getDescription().length() )columnWidths[ 2 ] = arg->getDescription().length();

            if( arg->visibleInHelp()
                && !arg->isRequired()
                && !arg->isValueRequired() )
                groupedShortOpts += arg->getFlag();
        }
        columnWidths[ 2 ] = _maxWidth - columnWidths[ 0 ] - columnWidths[ 1 ];
        std::string shortOpts = std::format( "{: ^{}}[{}{}]", "", columnWidths[ 0 ], TCLAP_FLAGSTARTCHAR,
                                             groupedShortOpts );

        // Long opts
        std::string groupOpts{};
        for( const auto& group : cmd.getArgGroups() ) {
            groupOpts += fmt::format( "\n{}:\n", group->getName() );
            std::string longOpts;
            for( const auto &arg : *group ) {
                std::string description{};
                for( auto word : std::ranges::split_view( arg->getDescription(), ' ' ) ) {
                    if( description.size() - description.find_last_of( '\n' ) >= columnWidths[ 2 ] ) {
                        description += std::format( "\n{: <{}}", "",
                                                    columnWidths[ 0 ] + columnWidths[ 1 ] + _indentWidth );
                    }
                    std::ranges::copy( word.begin(), word.end(), std::back_inserter( description ) );
                    description += " ";
                }
                longOpts += std::format(
                    "{: ^{}} {: <{}} {: <{}}\n",
                    arg->getFlag().empty() ? "" : "-" + arg->getFlag(), columnWidths[ 0 ],
                    "--" + arg->getName(), columnWidths[ 1 ],
                    description, columnWidths[ 2 ] );
            }
            groupOpts += longOpts;
        }


        fmt::println( "{}", cmd.getMessage() );
        fmt::println( "USAGE:\n{}", shortOpts );
        fmt::println( "Where:\n{}", groupOpts );
    }

    void version( CmdLineInterface& c ) override {
        fmt::println( "my version message: 0.1" );
    }
};
}
#endif //FMTOUTPUT_H
