#pragma once

#include <cstdint>

#include <OpenImageIO/imagebuf.h>

#include "tilemap.h"
#include "tilecache.h"

class TiledImage
{
    // == data members ==
    uint32_t _tileWidth = 32;
    uint32_t _tileHeight = 32;
public:
    TileMap tileMap; //!< tile map
    TileCache tileCache; //!< tile cache
    OpenImageIO::ImageBuf *currentTile = NULL; //!< 

    // read only references
    const uint32_t &tileWidth = _tileWidth; //!< Tile width
    const uint32_t &tileHeight = _tileHeight; //!< Tile hight

    // == constructors ==
    TiledImage( ); //!< default constructor
    TiledImage( uint32_t inWidth, uint32_t inHeight,
        uint32_t inTileWidth = 32, uint32_t inTileHeight = 32 );
    ~TiledImage(); //!<

    // == Modifications ==
    void setSize( uint32_t width, uint32_t height );
    void setTileSize( uint32_t width, uint32_t height );
    void setTileMap( TileMap tileMap );

    void mapFromCSV( std::string );

    /// == Generation ==
    void squareFromCache();

    // == Access ==
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
