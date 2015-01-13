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

TiledImage::TiledImage( uint32_t inWidth, uint32_t inHeight,
    uint32_t inTileWidth, uint32_t inTileHeight )
{
    setTileSize( inTileWidth, inTileHeight );
    setSize( inWidth, inHeight );
    tileMap.setSize( inWidth / inTileHeight, inHeight / inTileHeight );
}

// MODIFICATION
// ============

void
TiledImage::setTileMap( TileMap inTileMap )
{
    CHECK( inTileMap.width ) << "tilemap has no width";
    CHECK( inTileMap.height ) << "tilemap has no height";

    tileMap = inTileMap;
}

void
TiledImage::setSize( uint32_t inWidth, uint32_t inHeight )
{
    CHECK( inWidth >= tileWidth )
        << "width must be >= tile width (" << tileWidth << ")";
    CHECK(! (inWidth % tileWidth) )
        << "width must be a multiple of tile width (" << tileWidth << ")";

    CHECK( inHeight >= tileHeight )
        << "height must be >= tile height (" << tileHeight << ")";
    CHECK(! (inHeight % tileHeight) )
        << "height must be a multiple of tile height (" << tileHeight << ")";

    tileMap.setSize( inWidth / tileWidth, inHeight / tileHeight );
}

void
TiledImage::setTileSize( uint32_t inWidth, uint32_t inHeight )
{
    CHECK( inWidth > 0 ) << "width(" << inWidth << ") must be greater than zero";
    CHECK( !(inWidth % 4) ) << "width % 4 = " << inWidth % 4 << "!= 0 must be a multiple of four";
    CHECK( inHeight > 0 ) << "height(" << inHeight << ") must be greater than zero";
    CHECK( !(inHeight % 4) ) << "height % 4 = " << inHeight % 4 << "!= 0 must be a multiple of four";

    _tileWidth = inWidth;
    _tileHeight = inHeight;
}

void
TiledImage::mapFromCSV( std::string fileName )
{
    tileMap.fromCSV( fileName );
}

// GENERATION
// ==========
void
TiledImage::squareFromCache( )
{
    int tileCount = tileCache.getNTiles();
    CHECK( tileCount ) << "tileCache has no tiles";

    int square = sqrt(tileCount);
    tileMap.setSize( square, square );
    tileMap.consecutive();
}

// ACCESS
// ======

uint32_t
TiledImage::getWidth()
{
    return tileMap.width * tileWidth;
}

uint32_t
TiledImage::getHeight()
{
    return tileMap.height * tileHeight;
}

OpenImageIO::ImageBuf *
TiledImage::getRegion(
    uint32_t x1, uint32_t y1, // begin point
    uint32_t x2, uint32_t y2, // end point
    uint32_t sw, uint32_t sh) // scale width/height
{
    OIIO_NAMESPACE_USING;
    //FIXME needs to get region scaled rather than full size.
    LOG( INFO ) << "source window "
        << "(" << x1 << ", " << y1 << ")->(" << x2 << ", " << y2 << ")";

    CHECK( x1 < getWidth() ) << "x1 is out of range";
    CHECK( y1 < getHeight() ) << "y1 is out of range";
    if( x2 == 0 || x2 > getWidth() ) x2 = getWidth();
    if( y2 == 0 || y2 > getHeight() ) y2 = getHeight();
     LOG( INFO ) << "source window "
         << "(" << x1 << ", " << y1 << ")->(" << x2 << ", " << y2 << ")";

    if( sw == 0 ) sw = x2 - x1;
    if( sh == 0 ) sh = y2 - y1;

    ImageSpec spec( sw, sh, 4, TypeDesc::UINT8 );
    ImageBuf *dest = new ImageBuf( spec );
    //current point of interest
    uint32_t ix = x1;
    uint32_t iy = y1;
    while( true ){
         LOG( INFO ) << "Point of interest (" << ix << ", " << iy << ")";

        //determine the tile under the point of interest
        uint32_t mx = ix / tileWidth;
        uint32_t my = iy / tileHeight;

        //determine the top left corner of the copy window
        uint32_t wx1 = ix - mx * tileWidth;
        uint32_t wy1 = iy - my * tileHeight;

        //determine the bottom right corner of the copy window
        uint32_t wx2, wy2;
        if( x2 / tileWidth > mx ) wx2 = tileWidth;
        else wx2 = x2 - mx * tileWidth;

        if( y2 / tileHeight > my ) wy2 = tileHeight;
        else wy2 = y2 - my * tileHeight;

         LOG( INFO ) << "copy window "
             << "(" << wx1 << ", " << wy1 << ")->(" << wx2 << ", " << wy2 << ")";

        //determine the dimensions of the copy window
        uint32_t ww = wx2 - wx1;
        uint32_t wh = wy2 - wy1;
        LOG( INFO ) << "copy window size " << ww << "x" << wh;

        //determine the top left of the paste window
        uint32_t dx = ix - x1;
        uint32_t dy = iy - y1;
        LOG( INFO ) << "Paste Window: " << dx << "x" << dy;

        uint32_t index = tileMap(mx, my);
        ImageBuf *tile = tileCache.getScaled( index, tileWidth, tileHeight );
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
            tile->clear();
            delete tile;
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
        u1 * getWidth(), v1 * getHeight(),
        u2 * getWidth(), v2 * getHeight(),
        sw, sh );
}

OpenImageIO::ImageBuf *
TiledImage::getTile( uint32_t idx )
{
    return tileCache.getScaled( idx, tileWidth, tileHeight );
}
