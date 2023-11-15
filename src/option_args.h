#include <filesystem>
#include <spdlog/spdlog.h>
#include <optionparser.h>
#include <OpenImageIO/imageio.h>

// Argument tests //
////////////////////
auto isInteger( const std::string& string ) -> std::pair<bool, std::string> {
    try {
        std::stoi(string );
        return {true, {} };
    }
    catch( std::invalid_argument const& ex ){
        return {false, fmt::format("std::invalid_argument::what(): {}", ex.what() ) };
    }
    catch (std::out_of_range const& ex) {
        return {false, fmt::format( "std::out_of_range::what(): {}", ex.what() ) };
    }
}

auto isFile( const std::string& string ) -> std::pair<bool, std::string> {
    std::filesystem::path filePath( string );
    if(! std::filesystem::exists( filePath ) ) return {false, std::format("Unable to locate: {}", filePath.string() ) };
    if(! std::filesystem::is_regular_file(filePath ) ) return {false, std::format("Irregular file: {}", filePath.string() ) };
    return {true, ""};
}

auto isImage( const std::string& string ) -> std::pair<bool, std::string> {
    auto image = OIIO::ImageInput::open( string );
    if( image ){
        image->close();
        return {true,{} };
    }
    return {false,fmt::format("Unable to open image: {}", string ) };
}

struct Arg: public option::Arg
{
    static option::ArgStatus Unknown(const option::Option& option, bool msg) {
        if (msg) SPDLOG_ERROR( "Unknown option '{}'", option.name );
        return option::ARG_ILLEGAL;
    }

    static option::ArgStatus Required(const option::Option& option, bool msg) {
        if (option.arg != nullptr) {
            return option::ARG_OK;
        }
        if (msg) SPDLOG_ERROR( "Option '{}' requires an argument", option.name );
        return option::ARG_ILLEGAL;
    }

    static option::ArgStatus Integer(const option::Option& option, bool msg) {
        auto [result, errMsg ] = isInteger( option.arg );
        if( result ) return option::ARG_OK;
        if( msg ) SPDLOG_ERROR( errMsg );
        return option::ARG_ILLEGAL;
    }

    static option::ArgStatus Float(const option::Option& option, bool msg) {
        try {
            std::stof(option.arg);
            return option::ARG_OK;
        }
        catch (std::invalid_argument const& ex) {
            if( msg ) SPDLOG_ERROR( "std::invalid_argument::what(): {}", ex.what() );
            return option::ARG_ILLEGAL;
        }
        catch (std::out_of_range const& ex) {
            if( msg ) SPDLOG_ERROR( "std::out_of_range::what(): {}", ex.what() );
            return option::ARG_ILLEGAL;
        }
    }

    static option::ArgStatus Pixel8(const option::Option& option, bool msg) {
        int value = -1;
        try {
            value = std::stoi(option.arg);
        }
        catch (std::invalid_argument const& ex) {
            if( msg ) SPDLOG_ERROR( "std::invalid_argument::what(): {}", ex.what() );
            return option::ARG_ILLEGAL;
        }
        catch (std::out_of_range const& ex) {
            if( msg ) SPDLOG_ERROR( "std::out_of_range::what(): {}", ex.what() );
            return option::ARG_ILLEGAL;
        }
        if( value < 0 || value >= 0xFF ){
            if( msg ) SPDLOG_ERROR( "Value is out_of_range(0x00-0xFF): {:02x}", value );
            return option::ARG_ILLEGAL;
        }
        return option::ARG_OK;
    }

    static option::ArgStatus File( const option::Option& option, bool msg) {
        auto [result, errMsg ] = isFile( option.arg );
        if( result ) return option::ARG_OK;
        if( msg ) SPDLOG_ERROR( errMsg );
        return option::ARG_ILLEGAL;
    }

    static option::ArgStatus Image( const option::Option& option, bool msg) {
        auto [result, errMsg ] = isImage( option.arg );
        if( result ) return option::ARG_OK;
        if( msg ) SPDLOG_ERROR( errMsg );
        return option::ARG_ILLEGAL;
    }
};
