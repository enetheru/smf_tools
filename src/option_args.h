#include <fstream>
#include <cstring>

#include <OpenImageIO/imageio.h>
#include <optionparser.h>
#include <spdlog/spdlog.h>


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
        if (option.arg != nullptr)
            return option::ARG_OK;

        if (msg) printError("Option '", option, "' requires an argument\n");
        return option::ARG_ILLEGAL;
    }

    static option::ArgStatus Numeric(const option::Option& option, bool msg)
    {
        char* endptr = nullptr;
        if (option.arg != nullptr && strtof(option.arg, &endptr)){}
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
            spdlog::error( "Option '{}' cannot find file: {}" , option.name, option.arg );
        }
        return option::ARG_ILLEGAL;

    }

    static option::ArgStatus Image( const option::Option& option, bool msg)
    {
        auto image = OIIO::ImageInput::open( option.arg );
        if(! image ){
            spdlog::error( "Option '{}' cannot find file: {}", option.name, option.arg );
            return option::ARG_ILLEGAL;
        } else {
            image->close();
            return option::ARG_OK;
        }
    }
};
