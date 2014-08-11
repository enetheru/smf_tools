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
{
    pw = 1024;
    ph = 1024;
    tw = 32;
    th = 32;
    mw = pw / tw;
    mh = ph / th;

    tileMap.setSize( mw, mh );
}

TiledImage::TiledImage( uint32_t w, uint32_t h, uint32_t tw, uint32_t th )
    : pw( w ), ph( h ), tw( tw ), th( th )
{
    mw = pw / tw;
    mh = ph / th;

    tileMap.setSize( mw, mh );
    tileCache.setTileSize( tw );
}

// MODIFICATION
// ============

void
TiledImage::setTileMap( TileMap tm )
{
    tileMap = tm;
    mw = tileMap.width;
    mh = tileMap.height;
    pw = mw * tw;
    ph = mh * th;
}

void
TiledImage::setSize( uint32_t w, uint32_t h )
{
    pw = w;
    ph = h;
    mw = pw / tw;
    mh = ph / th;

    tileMap.setSize( mw, mh );
}

void
TiledImage::setTileSize( uint32_t w, uint32_t h )
{
    tw = w;
    th = h;
    mw = pw / tw;
    mh = ph / th;

    tileMap.setSize( mw, mh );
    tileCache.setTileSize( tw );
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
    tw = th = tileCache.getTileSize();
    uint32_t root = sqrt( tileCache.getNTiles() );
    if(! root ) root = 1;
    mw = mh = root;
    pw = ph = root * tw;
    tileMap.setSize( mw, mh );
    tileMap.consecutive();
}

// ACCESS
// ======
OpenImageIO::ImageBuf *
TiledImage::getRegion( uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2 )
{
    OIIO_NAMESPACE_USING;

    if( x1 > pw ){
        LOG(ERROR) << "x1 is out of range";
        return NULL;
    } 
    if( y1 > pw ){
        LOG(ERROR) << "y1 is out of range";
    }
    if( (x2 == 0) | (x2 > pw) ) x2 = pw;
    if( (y2 == 0) | (y2 > pw) ) y2 = ph;

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
        ImageBuf *tile = tileCache.getTile( index );
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
