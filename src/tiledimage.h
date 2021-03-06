#pragma once

#include <cstdint>

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>

#include "tilemap.h"
#include "tilecache.h"

class TiledImage
{
    // == data members ==
    std::unique_ptr< OpenImageIO::ImageBuf > currentTile; //!<
    OpenImageIO::ImageSpec _tSpec =
            OpenImageIO::ImageSpec( 32, 32, 4, OpenImageIO::TypeDesc::UINT8 );
    uint32_t _overlap = 0; //!< used for when tiles share border pixels

public:
    TileMap tileMap; //!< tile map
    TileCache tileCache; //!< tile cache

    // == constructors ==
    TiledImage( ) { }
    TiledImage( uint32_t inWidth, uint32_t inHeight,
        uint32_t inTileWidth = 32, uint32_t inTileHeight = 32 );

    // == Modifications ==
    void setSize( uint32_t width, uint32_t height );
    void setTileSize( uint32_t width, uint32_t height );
    void setTSpec( OpenImageIO::ImageSpec spec );
    void setTileMap( TileMap tileMap );
    void setOverlap( uint32_t overlap );

    void mapFromCSV( std::string );

    /// == Generation ==
    void squareFromCache();

    // == Access ==
    // read only references
    const OpenImageIO::ImageSpec &tSpec = _tSpec;
    const uint32_t &overlap = _overlap;

    // access methods
    uint32_t getWidth();
    uint32_t getHeight();

    /// Get pixel region
    /*  TODO
     *
     */
    std::unique_ptr< OpenImageIO::ImageBuf > getRegion(
            const OpenImageIO::ROI & );

    /// Get image Region, relative coords.
    /*  TODO
     *
     */
    std::unique_ptr< OpenImageIO::ImageBuf > getUVRegion(
            const uint32_t xbegin, const uint32_t xend,
            const uint32_t ybegin, const uint32_t yend );

    /// get raw tile image
    /*  TODO
     *
     */
    std::unique_ptr< OpenImageIO::ImageBuf > getTile( const uint32_t idx );
};
