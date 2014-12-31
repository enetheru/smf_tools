#include "tiledimage.h"
#include "tilemap.h"
#include "tilecache.h"

#include "elog/elog.h"
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <cstdint>

// CONSTRUCTORS
// ============
TiledImage::TiledImage( )
{ }

TiledImage::TiledImage( uint32_t w, uint32_t h, uint32_t tw, uint32_t th )
{
    setTileSize( tw, th );
    setSize( w, h );
    tileMap.setSize( w / tw, h / th );
}

// MODIFICATION
// ============

void
TiledImage::setTileMap( TileMap tm )
{
    CHECK( tm.width ) << "tilemap has no width";
    CHECK( tm.height ) << "tilemap has no height";

    tileMap = tm;
    w = tileMap.width * tw;
    h = tileMap.height * th;
}

void
TiledImage::setSize( uint32_t w, uint32_t h )
{
    CHECK( w >= tw )
        << "width must be >= tile width (" << tw << ")";
    CHECK(! (w % tw) )
        << "width must be a multiple of tile width (" << tw << ")";

    CHECK( h >= th )
        << "height must be >= tile height (" << th << ")";
    CHECK(! (h % th) )
        << "height must be a multiple of tile height (" << th << ")";

    this->w = w;
    this->h = h;

    tileMap.setSize( w / tw, h / th );
}

void
TiledImage::setTileSize( uint32_t w, uint32_t h )
{
    CHECK( w > 0 ) << "width(" << w << ") must be greater than zero";
    CHECK( !(w % 4) ) << "width % 4 = " << w % 4 << "!= 0 must be a multiple of four";
    CHECK( h > 0 ) << "height(" << h << ") must be greater than zero";
    CHECK( !(h % 4) ) << "height % 4 = " << h % 4 << "!= 0 must be a multiple of four";
    
    
    tw = w;
    th = h;
    this->w = tw * tileMap.width;
    this->h = th * tileMap.height;
}

void
TiledImage::mapFromCSV( std::string fileName )
{
    tileMap.fromCSV( fileName );
    w = tileMap.width * tw;
    h = tileMap.height * th;
}


// GENERATION
// ==========
void
TiledImage::squareFromCache( )
{
    int tc = tileCache.getNTiles();
    CHECK( tc ) << "tileCache has no tiles";
    
    int sq = sqrt(tc);
    tileMap.setSize( sq, sq );
    tileMap.consecutive();
    w = h = sq * tw;
    //FIXME set tilesize based on images sizes in cache
}

// ACCESS
// ======
OpenImageIO::ImageBuf *
TiledImage::getRegion(
    uint32_t x1, uint32_t y1, // begin point
    uint32_t x2, uint32_t y2, // end point
    uint32_t sw, uint32_t sh) // scale width/height
{
    OIIO_NAMESPACE_USING;
    //FIXME needs to get region scaled rather than full size.

    CHECK( x1 < w ) << "x1 is out of range";
    CHECK( y1 < w ) << "y1 is out of range";
    if( x2 == 0 || x2 > w ) x2 = w;
    if( y2 == 0 || y2 > h ) y2 = h;
    
    if( sw == 0 ) sw = x2 - x1;
    if( sh == 0 ) sh = y2 - y1;

    ImageSpec spec( sw, sh, 4, TypeDesc::UINT8 );
    ImageBuf *dest = new ImageBuf( spec );
    //current point of interest
    uint32_t ix = x1;
    uint32_t iy = y1;
    while( true ){

        //determine the tile under the point of interest
        uint32_t mx = ix / tw;
        uint32_t my = iy / th;

        //determine the top left corner of the copy window
        uint32_t wx1 = ix - mx * tw;
        uint32_t wy1 = iy - my * th;

        //determine the bottom right corner of the copy window
        uint32_t wx2 = std::min( tw, x2 - ix );
        uint32_t wy2 = std::min( th, y2 - iy );

        //determine the dimensions of the copy window
        uint32_t ww = wx2 - wx1;
        uint32_t wh = wy2 - wy1;

        //determine the top left of the paste window
        uint32_t dx = ix - x1;
        uint32_t dy = iy - y1;

        uint32_t index = tileMap(mx, my);
        ImageBuf *tile = tileCache.getScaled( index, tw, th );
        if( tile ){
            //copy pixel data from source tile to dest
            ROI window;
            window.xbegin = wx1;
            window.xend = wx2;
            window.ybegin = wy1;
            window.yend = wy2;
            window.zbegin = 0;
            window.zend = 1;
            window.chbegin = 0;
            window.chend = 4;
            ImageBufAlgo::paste( *dest, dx, dy, 0, 0, *tile, window );
        }

        //determine the next point of interest
        ix += ww;
        if( ix >= x2 ){
            ix = x1;
            iy += wh;
            if( iy >= y2 ){
                //then weve copied all the data and exit;
                break;
            }
        }
    }
    return dest;
}

OpenImageIO::ImageBuf *
TiledImage::getUVRegion(
    float u1, float v1, // begin point
    float u2, float v2, // end point
    uint32_t sw, uint32_t sh ) // scale width/height
{
    return getRegion(
        u1 * w, v1 * h,
        u2 * w, v2 * h,
        sw, sh );
}

OpenImageIO::ImageBuf *
TiledImage::getTile( uint32_t idx )
{
    return tileCache.getScaled( idx, tw, th );
}
