#pragma once

#include <vector>
#include <string>
#include <memory>

#include <OpenImageIO/imagebuf.h>

class TileCache
{
    // member data
    uint32_t _nTiles = 0;
    // FIXME this twin vector mapping can be turned into a pair or a tuple
    std::vector< uint32_t > map;
    std::vector< std::string > fileNames;

public:
    // data access
    const uint32_t &nTiles = _nTiles;

    // modifications
    void addSource( const std::string& );

    /// get a tile from the cache
    /*
     *
     */
    std::unique_ptr< OIIO::ImageBuf > getTile(uint32_t n);

    TileCache &operator=( const TileCache& rhs ){
        _nTiles = rhs._nTiles;
        map = rhs.map;
        fileNames = rhs.fileNames;
        return *this;
    }
};
