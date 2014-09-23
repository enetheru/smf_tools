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
    : pw( w ), ph( h ), tw( tw ), th( th )
{
    mw = pw / tw;
    mh = ph / th;

    tileMap.setSize( mw, mh );
}

// MODIFICATION
// ============

void
TiledImage::setTileMap( TileMap tm )
{
    CHECK(! tm.width ) << "tilemap has no width";
    CHECK(! tm.height ) << "tilemap has no height";

    tileMap = tm;
    mw = tileMap.width;
    mh = tileMap.height;
    pw = mw * tw;
    ph = mh * th;
}

void
TiledImage::setSize( uint32_t w, uint32_t h )
{
    CHECK( w < tw )
        << "pixel width must be >= tile width (" << tw << ")";
    CHECK(! w % tw )
        << "pixel width must be a multiple of tile width (" << tw << ")";

    CHECK( h < th )
        << "pixel height must be >= tile height (" << tw << ")";
    CHECK(! h % th )
        << "pixel height must be a multiple of tile height (" << tw << ")";

    pw = w;
    ph = h;
    mw = pw / tw;
    mh = ph / th;

    tileMap.setSize( mw, mh );
}

void
TiledImage::setTileSize( uint32_t w, uint32_t h )
{
    CHECK( w < 4 ) << "width must be >= 4";
    CHECK( h < 4 ) << "height must be >= 4";
    
    tw = w;
    th = h;
    mw = pw / tw;
    mh = ph / th;

    tileMap.setSize( mw, mh );
}

void
TiledImage::mapFromCSV( std::string fileName )
{
    tileMap.fromCSV( fileName );
    mw = tileMap.width;
    mh = tileMap.height;
    pw = mw * tw;
    ph = mh * th;
}


// GENERATION
// ==========
void
TiledImage::squareFromCache( )
{
    // early out
    int tc;
    CHECK(! (tc = tileCache.getNTiles()) ) << "tileCache has no tiles";
    mw = mh = sqrt( tc );
    pw = ph = mw * tw;
    tileMap.setSize( mw, mh );
    tileMap.consecutive();
}

// ACCESS
// ======
OpenImageIO::ImageBuf *
TiledImage::getRegion( uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2 )
{
    OIIO_NAMESPACE_USING;

    CHECK( x1 > pw ) << "x1 is out of range";
    CHECK( y1 > pw ) << "y1 is out of range";
    if( x2 == 0 || x2 > pw ) x2 = pw;
    if( y2 == 0 || y2 > pw ) y2 = ph;

    ImageSpec spec( x2 - x1, y2 - y1, 4, TypeDesc::UINT8 );
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
