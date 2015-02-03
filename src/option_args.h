#include <fstream>
#include <cstring>

#include "optionparser/optionparser.h"
#include "elog/elog.h"


// Argument tests //
////////////////////
struct Arg: public option::Arg
{
    static void printError(const char* msg1, const option::Option& opt,
            const char* msg2)
    {
        fprintf(stderr, "%s", msg1);
        fwrite(opt.name, opt.namelen, 1, stderr);
        fprintf(stderr, "%s", msg2);
    }

    static option::ArgStatus Unknown(const option::Option& option, bool msg)
    {
        if (msg) printError("Unknown option '", option, "'\n");
        return option::ARG_ILLEGAL;
    }

    static option::ArgStatus Required(const option::Option& option, bool msg)
    {
        if (option.arg != 0)
            return option::ARG_OK;

        if (msg) printError("Option '", option, "' requires an argument\n");
        return option::ARG_ILLEGAL;
    }

    static option::ArgStatus Numeric(const option::Option& option, bool msg)
    {
        char* endptr = 0;
        if (option.arg != 0 && strtof(option.arg, &endptr)){};
        if (endptr != option.arg && *endptr == 0)
            return option::ARG_OK;

        if (msg) printError("Option '", option,
                "' requires a numeric argument\n");
        return option::ARG_ILLEGAL;
    }

    static option::ArgStatus File( const option::Option& option, bool msg)
    {
        std::fstream tempFile( option.arg, std::ios::in );
        if( tempFile.good() ){
            tempFile.close();
            return option::ARG_OK;
        }
        tempFile.close();

        if (msg) {
            LOG( ERROR ) << "Option '" << option.name << "' cannot find file: " << option.arg;
        }
        return option::ARG_ILLEGAL;
        
    }
};
