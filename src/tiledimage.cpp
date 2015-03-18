#include <cstdint>

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>

#include "elog/elog.h"

#include "tiledimage.h"
#include "tilemap.h"
#include "tilecache.h"

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

TiledImage::~TiledImage( )
{
    if( currentTile ){
        currentTile->clear();
        delete currentTile;
    }
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
    CHECK( inWidth >= (uint32_t)tSpec.width )
        << "width must be >= tile width (" << tSpec.width << ")";

    CHECK( inHeight >= (uint32_t)tSpec.height )
        << "height must be >= tile height (" << tSpec.height << ")";

    tileMap.setSize( inWidth / tSpec.width, inHeight / tSpec.height );
}

void 
TiledImage::setTSpec( OpenImageIO::ImageSpec spec )
{
    _tSpec = spec;
}

void
TiledImage::setTileSize( uint32_t inWidth, uint32_t inHeight )
{
    CHECK( inWidth > 0 ) << "width(" << inWidth << ") must be greater than zero";
    CHECK( inHeight > 0 ) << "height(" << inHeight << ") must be greater than zero";

    _tSpec.width = inWidth;
    _tSpec.height = inHeight;
}

void
TiledImage::setOverlap( uint32_t overlap )
{
    _overlap = overlap;
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
    return tileMap.width * (tSpec.width - overlap) + overlap;
}

uint32_t
TiledImage::getHeight()
{
    return tileMap.height * (tSpec.height - overlap) + overlap;
}

OpenImageIO::ImageBuf *
TiledImage::getRegion(
    uint32_t x1, uint32_t y1, // begin point
    uint32_t x2, uint32_t y2, // end point
    uint32_t sw, uint32_t sh) // scale width/height
{
    OIIO_NAMESPACE_USING;
    //FIXME needs to get region scaled rather than full size.
    DLOG( INFO ) << "source window "
        << "(" << x1 << ", " << y1 << ")->(" << x2 << ", " << y2 << ")";

    CHECK( x1 < getWidth() ) << "x1 is out of range";
    CHECK( y1 < getHeight() ) << "y1 is out of range";
    if( x2 == 0 || x2 > getWidth() ) x2 = getWidth();
    if( y2 == 0 || y2 > getHeight() ) y2 = getHeight();
     DLOG( INFO ) << "source window "
         << "(" << x1 << ", " << y1 << ")->(" << x2 << ", " << y2 << ")";

    if( sw == 0 ) sw = x2 - x1;
    if( sh == 0 ) sh = y2 - y1;

    static ImageSpec spec( sw, sh, tSpec.nchannels, tSpec.format );
    spec.width = sw;
    spec.height = sh;

    ImageBuf *dest = new ImageBuf( spec );
    //current point of interest
    bool hasData = false;
    uint32_t ix = x1;
    uint32_t iy = y1;
    static uint32_t index_p = INT_MAX;
    while( true ){
         DLOG( INFO ) << "Point of interest (" << ix << ", " << iy << ")";

        //determine the tile under the point of interest
        uint32_t mx = ix / (tSpec.width - overlap);
        uint32_t my = iy / (tSpec.height - overlap);

        //determine the top left corner of the copy window
        uint32_t wx1 = ix - mx * (tSpec.width - overlap);
        uint32_t wy1 = iy - my * (tSpec.height - overlap);

        //determine the bottom right corner of the copy window
        uint32_t wx2, wy2;
        if( x2 / (tSpec.width - overlap) > mx ) wx2 = tSpec.width;
        else wx2 = x2 - mx * (tSpec.width - overlap);

        if( y2 / (tSpec.height - overlap) > my ) wy2 = tSpec.height;
        else wy2 = y2 - my * (tSpec.height - overlap);

         DLOG( INFO ) << "copy window "
             << "(" << wx1 << ", " << wy1 << ")->(" << wx2 << ", " << wy2 << ")";

        //determine the dimensions of the copy window
        uint32_t ww = wx2 - wx1;
        uint32_t wh = wy2 - wy1;
        DLOG( INFO ) << "copy window size " << ww << "x" << wh;

        //determine the top left of the paste window
        uint32_t dx = ix - x1;
        uint32_t dy = iy - y1;
        DLOG( INFO ) << "Paste Window: " << dx << "x" << dy;

        uint32_t index = tileMap(mx, my);
        if( index != index_p ){
            if( currentTile ){currentTile->clear(); delete currentTile;}
            currentTile = tileCache.getSpec( index, tSpec );
            index_p = index;
        }
        if( currentTile ){
            hasData = true;
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
            ImageBufAlgo::paste( *dest, dx, dy, 0, 0, *currentTile, window );
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

    if( hasData )return dest;
    return NULL;
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
    return tileCache.getSpec( idx, tSpec );
}
