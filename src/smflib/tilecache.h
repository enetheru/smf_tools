#pragma once

#include <vector>
#include <string>
#include <memory>

#include <OpenImageIO/imagebuf.h>
#include <filesystem>
#include <nlohmann/json.hpp>

enum class TileSourceType {
    Image,SMT,SMF,None
};

std::string to_string( TileSourceType type );

class TileCache
{
    struct TileSource{
        uint32_t iStart{},iEnd{};
        TileSourceType type = TileSourceType::None;
        std::filesystem::path filePath{};
        [[nodiscard]] nlohmann::ordered_json json() const {
            nlohmann::ordered_json j;
            j["start"] = iStart;
            j["end"] = iEnd;
            j["type"] = to_string(type);
            j["path"] = filePath.string();
            return j;
        }
    };
    // member data
    uint32_t _numTiles = 0;
    std::vector<TileSource> _sources;

public:
    TileCache() = default;
    TileCache( TileCache &&other ) noexcept;    // move constructor
    TileCache( const TileCache &other );        // copy constructor

    TileCache& operator=( const TileCache &other );     // copy assignment
    TileCache& operator=( TileCache &&other ) noexcept; // move assignment

    // data access
    [[nodiscard]] uint32_t getNumTiles() const { return _numTiles; }

    // modifications
    void addSource( const std::filesystem::path& filePath );
    [[nodiscard]] auto getSources() const -> const auto & { return _sources; }

    /// get a tile from the cache
    [[nodiscard]] std::optional<OIIO::ImageBuf> getTile( uint32_t index ) const ;

    [[nodiscard]] nlohmann::ordered_json json() const ;
};
