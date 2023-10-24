#pragma once

#include <vector>
#include <string>
#include <memory>

#include <OpenImageIO/imagebuf.h>

enum class TileSourceType {
    Image,SMT,SMF
};

class TileCache
{
    struct TileSource{
        uint32_t iStart,iEnd;
        TileSourceType type;
        std::filesystem::path filePath;
    };
    // member data
    uint32_t tileCount = 0;
    std::vector<TileSource> sources;

public:
    // data access
    uint32_t getNumTiles() const { return tileCount; }

    // modifications
    void addSource( const std::filesystem::path& filePath );

    /// get a tile from the cache
    std::optional<OIIO::ImageBuf> getTile( uint32_t index );
};
