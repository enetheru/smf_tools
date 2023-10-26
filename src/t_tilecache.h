#pragma once

#include <vector>
#include <string>
#include <memory>

#include <OpenImageIO/imagebuf.h>
#include <filesystem>

enum class TileSourceType {
    Image,SMT,SMF
};

std::string to_string( TileSourceType type );

class TileCache
{
    struct TileSource{
        uint32_t iStart,iEnd;
        TileSourceType type;
        std::filesystem::path filePath;
    };
    // member data
    uint32_t _numTiles = 0;
    std::vector<TileSource> sources;

public:
    // data access
    [[nodiscard]] uint32_t getNumTiles() const { return _numTiles; }

    // modifications
    void addSource( const std::filesystem::path& filePath );

    /// get a tile from the cache
    std::optional<OIIO::ImageBuf> getTile( uint32_t index );
};
