#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>

#include <elog.h>

#include <utility>

#include "smf_tools.h"
#include "tiledimage.h"
#include "util.h"

OIIO_NAMESPACE_USING

// CONSTRUCTORS
// ============
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
TiledImage::setTileMap( const TileMap& inTileMap )
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
TiledImage::setTSpec( ImageSpec spec )
{
    _tSpec = std::move(spec);
}

void
TiledImage::setTileSize( uint32_t inWidth, uint32_t inHeight )
{
    CHECK( inWidth > 0 ) << "width(" << inWidth << ") must be greater than zero";
    CHECK( inHeight > 0 ) << "height(" << inHeight << ") must be greater than zero";
    _tSpec = ImageSpec( inWidth, inHeight, _tSpec.nchannels, _tSpec.format );
}

void
TiledImage::setOverlap( uint32_t overlap )
{
    _overlap = overlap;
}

void
TiledImage::mapFromCSV( const std::string& fileName )
{
    tileMap.fromCSV( fileName );
}

// GENERATION
// ==========
void
TiledImage::squareFromCache( )
{
    int tileCount = tileCache.nTiles;
    CHECK( tileCount ) << "tileCache has no tiles";

    int square = sqrt(tileCount);
    tileMap.setSize( square, square );
    tileMap.consecutive();
}

// ACCESS
// ======

uint32_t
TiledImage::getWidth() const
{
    return tileMap.width * (tSpec.width - overlap) + overlap;
}

uint32_t
TiledImage::getHeight() const
{
    return tileMap.height * (tSpec.height - overlap) + overlap;
}

std::unique_ptr< ImageBuf >
TiledImage::getRegion(
    const ROI &roi )
{
    DLOG( INFO ) << "source window "
        << "(" << roi.xbegin << ", " << roi.ybegin << ")"
      << "->(" << roi.xend   << ", " << roi.yend   << ")";

    ImageSpec outSpec( roi.width(), roi.height(), 4, TypeDesc::UINT8 );

    std::unique_ptr< ImageBuf > outBuf( new ImageBuf( outSpec ) );
    //outBuf->write( "TiledImage_getRegion_outBuf.tif", "tif" );

    //current point of interest
    uint32_t ix = roi.xbegin;
    uint32_t iy = roi.ybegin;
    static uint32_t index_p = INT_MAX;
    ROI cw{0,0,0,0,0,1,0,4}; // copy window
    while( true ){
         DLOG( INFO ) << "Point of interest (" << ix << ", " << iy << ")";

        //determine the tile under the point of interest
        uint32_t mx = ix / (tSpec.width - overlap);
        uint32_t my = iy / (tSpec.height - overlap);

        //determine the top left corner of the copy window
        cw.xbegin = ix - mx * (tSpec.width - overlap);
        cw.ybegin = iy - my * (tSpec.height - overlap);

        //determine the bottom right corner of the copy window
        if( roi.xend / (tSpec.width - overlap) > mx ) cw.xend = tSpec.width;
        else cw.xend = roi.xend - mx * (tSpec.width - overlap);

        if( roi.yend / (tSpec.height - overlap) > my ) cw.yend = tSpec.height;
        else cw.yend = roi.yend - my * (tSpec.height - overlap);

         DLOG( INFO ) << "copy window "
             << "(" << cw.xbegin << ", " << cw.ybegin << ")"
            <<"->(" << cw.xend   << ", " << cw.yend   << ")";

        //determine the dimensions of the copy window
        DLOG( INFO ) << "copy window size " << cw.width() << "x" << cw.height();

        //determine the top left of the paste window
        uint32_t dx = ix - roi.xbegin;
        uint32_t dy = iy - roi.ybegin;
        DLOG( INFO ) << "Paste position: " << dx << "x" << dy;

        //Optimisation: exact copy of previous tile test
        uint32_t index = tileMap(mx, my);
        if( index != index_p ){
            // create blank tile if index is out of range
            if( index >= tileCache.nTiles ){
                currentTile.reset( new OIIO::ImageBuf( tSpec ) );
            } else {
                currentTile = tileCache.getTile(index);
                // A possibility exists that the tile cache will give us a tile that
                // has dimensions other than expected. So let's check and scale.
                currentTile = fix_scale( std::move( currentTile ), tSpec );
                // it's possible that the tile retrieved has different channels
                // than the tiledImage spec
                currentTile = fix_channels( std::move( currentTile ), tSpec );
            }
            index_p = index;
        }
        if( currentTile ){
            //copy pixel data from source tile to dest
            ImageBufAlgo::paste( *outBuf, dx, dy, 0, 0, *currentTile, cw );
            //outBuf->write( "TiledImage_getRegion_outBuf_paste.tif", "tif");
        }

        //determine the next point of interest
        ix += cw.width();
        if( int( ix ) >= roi.xend ){
            ix = roi.xbegin;
            iy += cw.height();
            if( int( iy ) >= roi.yend ){
                //then we've copied all the data and exit;
                break;
            }
        }
    }
#ifdef DEBUG_IMG
    outBuf->write( "TiledImage.getRegion.tif", "tif" );
#endif
    return outBuf ;
}

std::unique_ptr< ImageBuf >
TiledImage::getUVRegion(
        const uint32_t xbegin, const uint32_t xend,
        const uint32_t ybegin, const uint32_t yend )
{
    ROI roi(xbegin, xend, ybegin, yend );
    return getRegion( roi );
}

std::unique_ptr< ImageBuf >
TiledImage::getTile( const uint32_t idx )
{
    auto retval = tileCache.getTile( idx );
    retval = fix_channels( std::move( currentTile ), tSpec );
    retval = fix_scale( std::move( currentTile ), tSpec );
    return retval;
}
