#pragma once

#include <cstdint>

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>
#include <filesystem>

#include "tilemap.h"
#include "tilecache.h"

class TiledImage {
    // == data members ==
    OIIO::ImageBuf currentTile;
    OIIO::ImageSpec _tSpec = OIIO::ImageSpec( 32, 32, 4, OIIO::TypeDesc::UINT8 );
    uint32_t _overlap = 0; //!< used for when tiles share border pixels

public:
    TileMap tileMap;
    TileCache tileCache;
    [[nodiscard]] const OIIO::ImageSpec &getTileSpec() const { return _tSpec; }

    // == constructors ==
    TiledImage( ) = default;
    TiledImage( uint32_t inWidth, uint32_t inHeight,
        int inTileWidth = 32, int inTileHeight = 32 );

    // == Modifications ==
    void setSize( uint32_t width, uint32_t height );
    void setTileSize( int width, int height );
    void setTSpec( OIIO::ImageSpec spec );
    void setTileMap( const TileMap& tileMap );
    void setOverlap( uint32_t overlap );

    void mapFromCSV( std::filesystem::path filePath );

    /// == Generation ==
    void squareFromCache();

    // access methods
    [[nodiscard]] uint32_t getWidth() const;
    [[nodiscard]] uint32_t getHeight() const;

    OIIO::ImageBuf getRegion( const OIIO::ROI & );

    OIIO::ImageBuf getUVRegion(
            uint32_t xbegin, uint32_t xend,
            uint32_t ybegin, uint32_t yend );

    OIIO::ImageBuf getTile( uint32_t idx );
};
