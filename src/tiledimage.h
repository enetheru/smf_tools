#pragma once

#include <cstdint>

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>

#include "tilemap.h"
#include "tilecache.h"

class TiledImage
{
    // == data members ==
    OpenImageIO::ImageBuf *currentTile = NULL; //!< 
    OpenImageIO::ImageSpec _tSpec = OpenImageIO::ImageSpec( 32, 32, 4, OpenImageIO::TypeDesc::UINT8 );
    uint32_t _overlap = 0; //!< used for when tiles share border pixels

public:
    TileMap tileMap; //!< tile map
    TileCache tileCache; //!< tile cache

    // == constructors ==
    TiledImage( ); //!< default constructor
    TiledImage( uint32_t inWidth, uint32_t inHeight,
        uint32_t inTileWidth = 32, uint32_t inTileHeight = 32 );
    ~TiledImage(); //!< default destructor

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

    // Get pixel region
    OpenImageIO::ImageBuf *getRegion(
            uint32_t x1 = 0, uint32_t y1 = 0,
            uint32_t x2 = 0, uint32_t y2 = 0,
            uint32_t sw = 0, uint32_t sh = 0);

    // Get image Region, relative coords.
    OpenImageIO::ImageBuf *getUVRegion(
            float u1 = 0, float v1 = 0,
            float u2 = 1.0, float v2 = 1.0,
            uint32_t sw = 0, uint32_t sh = 0 );

    OpenImageIO::ImageBuf *getTile( uint32_t idx );
};
