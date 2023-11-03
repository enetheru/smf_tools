#pragma once

#include <cstdint>

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>
#include <filesystem>
#include <utility>

#include "tilemap.h"
#include "tilecache.h"

/*
 * TiledImage is made from a set of tiles stored in the TileCache,
 * and a TileMap which describes how to reconstruct them
 */

class TiledImage {
    TileCache _tileCache;
    TileMap _tileMap;

    OIIO::ImageSpec _imageSpec = OIIO::ImageSpec(0,0,4,OIIO::TypeDesc::UINT8 );
    uint32_t _tileWidth{}, _tileHeight{}, _tileOverlap{};

public:
    //[[nodiscard]] const OIIO::ImageSpec &getTileSpec() const { return _tileSpec; }
    [[nodiscard]] const OIIO::ImageSpec &getImageSpec() const { return _imageSpec; };

    // == constructors ==
    TiledImage( ) = default;
    TiledImage( TileCache tileCache, const TileMap& tileMap );
    explicit TiledImage( const OIIO::ImageSpec& imageSpec, uint32_t tileWidth = 32, uint32_t tileHeight = 32, uint32_t tileOverlap = 0 );

    void setImageSpec( const OIIO::ImageSpec& imageSpec );
    void setTileSize( uint32_t width, uint32_t height, uint32_t overlap = 0 );
    void setTileMap( const TileMap& tileMap );

    void setTileCache( const TileCache& tileCache ) { _tileCache = tileCache; }

    /// == Generation ==
    void squareFromCache();

    OIIO::ImageBuf getRegion( const OIIO::ROI & );

    OIIO::ImageBuf getUVRegion( OIIO::ROI roi );
};
