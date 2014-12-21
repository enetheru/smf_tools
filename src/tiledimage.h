#ifndef TILEDIMAGE_H
#define TILEDIMAGE_H

#include "tilemap.h"
#include "tilecache.h"

#include <cstdint>
#include <OpenImageIO/imagebuf.h>

class TiledImage
{
public:
    // data members
    TileMap tileMap; //< tile map
    TileCache tileCache; //< tile cache

    // numtiles_x: tileMap.width
    // numtiles_y: tileMap.height
    uint32_t tw = 32; //< Tile width
    uint32_t th = 32; //< Tile hight
    uint32_t w; //< Image Width
    uint32_t h; //< Image Hight

    //constructors
    TiledImage( ); //< default constructor
    TiledImage( uint32_t w, uint32_t h, uint32_t tw = 32, uint32_t th = 32 );

    // modifications
    void setSize( uint32_t w, uint32_t h );
    void setTileSize( uint32_t w, uint32_t h );
    void setTileMap( TileMap tileMap );

    void mapFromCSV( std::string );

    /// generation
    void squareFromCache();

    // data access
    // Get pixel region
    OpenImageIO::ImageBuf *getRegion(
            uint32_t x1 = 0, uint32_t y1 = 0,
            uint32_t x2 = 0, uint32_t y2 = 0 );

    // Get image Region, relative coords.
    OpenImageIO::ImageBuf *getUVRegion(
            float u1 = 0, float v1 = 0,
            float u2 = 1.0, float v2 = 1.0);

    OpenImageIO::ImageBuf *getTile( uint32_t idx );
};

#endif //TILEDIMAGE_H
