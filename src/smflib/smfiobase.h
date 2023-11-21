#ifndef FILEIO_H
#define FILEIO_H

// Project Includes
#include "filemap.h"

// Library Includes
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

// std includes
#include <utility>
#include <filesystem>
#include <fstream>
/**
 * \brief Writer base class for implementing writer components to smf file foramt
 * \tparam T - type of file to generate from reading smf component.
 */

namespace smflib {
class SMF;

class SMFIOBase {
protected:
    bool error{};
    std::string errorMsg{};

    SMF *_smf{};
    std::shared_ptr<FileMap> _fileMap;
    std::streamoff _position{};
    std::streamsize _bytes{};

    SMFIOBase() = default;
    SMFIOBase( const std::streamoff position, const std::streamsize size )
    : _position( position ), _bytes( size ) {}

    virtual ~SMFIOBase() = default;
public:
    virtual void read( std::ifstream& file ) = 0;
    virtual size_t write( std::ofstream& file ) = 0;
    virtual void update() = 0;
    virtual void reset() = 0;
    [[nodiscard]] virtual nlohmann::ordered_json json() = 0;
    virtual void setSMF( SMF *smf) { _smf = smf; }

    void setPosition( const std::streamoff position ) {_position = position; }
    void setSize( const std::streamsize size ) {_bytes = size; }
};

/**
 * \brief ZeroWriter writes zeroes to file and reads nothing.
 */
class ZeroWriter final : public SMFIOBase {
    using Path = std::filesystem::path;
    Path _path;
public:
    explicit ZeroWriter( Path path, const std::streamoff position, const std::streamsize size )
    : SMFIOBase( position, size ), _path(std::move(path)) {}

    void read( std::ifstream& file ) override { }

    size_t write( std::ofstream& file ) override {
        file.seekp( _position );
        if( file.fail() ) {
            SPDLOG_ERROR( "Unable to seep to file position: {}", _position );
            return 0;
        }
        constexpr char zero = 0;
        for( decltype(_bytes) i = 0; i < _bytes; ++i ) {
            file.write( &zero, 1);
        }
        return _bytes;
    }

    void update() override{}
    void reset() override;
    [[nodiscard]] nlohmann::ordered_json json() override {
        nlohmann::ordered_json j;
        return j;
    }
};

}

#endif //FILEIO_H
