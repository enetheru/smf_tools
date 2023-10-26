#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>

#include <spdlog/spdlog.h>

#include <utility>

#include "tiledimage.h"
#include "util.h"

// CONSTRUCTORS
// ============
TiledImage::TiledImage( uint32_t inWidth, uint32_t inHeight,
    int inTileWidth, int inTileHeight )
{
    setTileSize( inTileWidth, inTileHeight );
    setSize( inWidth, inHeight );
    tileMap.setSize( inWidth / inTileHeight, inHeight / inTileHeight );
}

// MODIFICATION
// ============

//FIXME, function can error, but does not notify the caller.
void
TiledImage::setTileMap( const TileMap& inTileMap )
{
    const auto [ width, height ] = inTileMap.size();
    if( !(width && height) ){
        SPDLOG_CRITICAL( "tilemap width or height is invalid: {}x{}", width, height );
        return;
    }
    tileMap = inTileMap;
}

//FIXME, function can error, but does not notify the caller
void
TiledImage::setSize( uint32_t inWidth, uint32_t inHeight )
{
    if( (int)inWidth >= _tSpec.width || (int)inHeight >= _tSpec.height ){
        SPDLOG_CRITICAL( "in:{}x{} must be >= tile: {}x{}",
                          inWidth, inHeight, _tSpec.width, _tSpec.height );
        return;
    }
    tileMap.setSize( inWidth / _tSpec.width, inHeight / _tSpec.height );
}

void
TiledImage::setTSpec( OIIO::ImageSpec spec )
{
    _tSpec = std::move(spec);
}

//FIXME function can error but does not notify the caller.
void
TiledImage::setTileSize( int inWidth, int inHeight )
{
    if( inWidth > 0 || inHeight > 0 ){
        SPDLOG_CRITICAL( "in:{}x{} must be > tile: 0x0",
                          inWidth, inHeight, _tSpec.width, _tSpec.height );
        return;
    }
    _tSpec = OIIO::ImageSpec( inWidth, inHeight, _tSpec.nchannels, _tSpec.format );
}

void
TiledImage::setOverlap( uint32_t overlap ) {
    _overlap = overlap;
}

void
TiledImage::mapFromCSV( std::filesystem::path filePath ) {
    tileMap.fromCSV( std::move(filePath) );
}

// GENERATION
// ==========
//FIXME function can error but does not notify the caller.
void
TiledImage::squareFromCache( ) {
    if( !tileCache.getNumTiles() ){
        SPDLOG_CRITICAL("tileCache has no tiles");
        return;
    }
    auto square = (uint32_t)sqrt(tileCache.getNumTiles());
    tileMap.setSize( square, square );
    tileMap.consecutive();
}

// ACCESS
// ======

uint32_t
TiledImage::getWidth() const {
    return tileMap.width() * (_tSpec.width - _overlap) + _overlap;
}

uint32_t
TiledImage::getHeight() const {
    return tileMap.height() * (_tSpec.height - _overlap) + _overlap;
}

OIIO::ImageBuf
TiledImage::getRegion( const OIIO::ROI &roi ) {
    SPDLOG_INFO( "source window ({}, {})->({}}, {}})", roi.xbegin, roi.ybegin, roi.xend, roi.yend );

    OIIO::ImageSpec outSpec( roi.width(), roi.height(), 4, OIIO::TypeDesc::UINT8 );

    OIIO::ImageBuf outBuf( outSpec );
    //outBuf->write( "TiledImage_getRegion_outBuf.tif", "tif" );

    //current point of interest
    uint32_t ix = roi.xbegin;
    uint32_t iy = roi.ybegin;
    static uint32_t index_p = INT_MAX;
    OIIO::ROI cw{0,0,0,0,0,1,0,4}; // copy window
    while( true ){
         SPDLOG_INFO( "Point of interest ({}, {})", ix, iy );

        //determine the tile under the point of interest
        uint32_t mx = ix / (_tSpec.width - _overlap);
        uint32_t my = iy / (_tSpec.height - _overlap);

        //determine the top left corner of the copy window
        cw.xbegin = ix - mx * (_tSpec.width - _overlap);
        cw.ybegin = iy - my * (_tSpec.height - _overlap);

        //determine the bottom right corner of the copy window
        if( roi.xend / (_tSpec.width - _overlap) > mx ) cw.xend = _tSpec.width;
        else cw.xend = roi.xend - mx * (_tSpec.width - _overlap);

        if( roi.yend / (_tSpec.height - _overlap) > my ) cw.yend = _tSpec.height;
        else cw.yend = roi.yend - my * (_tSpec.height - _overlap);

        SPDLOG_INFO( "copy window ({}, {})->({}}, {}})", cw.xbegin, cw.ybegin, cw.xend, cw.yend );

        //determine the dimensions of the copy window
        SPDLOG_INFO( "copy window size {}x{}", cw.width(), cw.height() );

        //determine the top left of the paste window
        uint32_t dx = ix - roi.xbegin;
        uint32_t dy = iy - roi.ybegin;
        SPDLOG_INFO( "Paste position: {}x{}", dx, dy );

        //Optimisation: exact copy of previous tile test
        uint32_t index = tileMap.getXY(mx, my);
        if( index != index_p ){
            // create blank tile if index is out of range
            if( index >= tileCache.getNumTiles() ){
                currentTile.reset( _tSpec );
            } else {
                currentTile = tileCache.getTile(index).value();
                // A possibility exists that the tile cache will give us a tile that
                // has dimensions other than expected. So let's check and scale.
                //currentTile = fix_scale( std::move( currentTile ), _tSpec );
                currentTile = scale( currentTile, _tSpec );
                // it's possible that the tile retrieved has different channels
                // than the tiledImage spec
                currentTile = channels( currentTile, _tSpec );
            }
            index_p = index;
        }
        if( currentTile.initialized() ){
            //copy pixel data from source tile to dest
            OIIO::ImageBufAlgo::paste( outBuf, dx, dy, 0, 0, currentTile, cw );
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

OIIO::ImageBuf
TiledImage::getUVRegion(
        const uint32_t xbegin, const uint32_t xend,
        const uint32_t ybegin, const uint32_t yend ) {
    OIIO::ROI roi(xbegin, xend, ybegin, yend );
    return getRegion( roi );
}

OIIO::ImageBuf
TiledImage::getTile( const uint32_t idx ) {
    auto retval = tileCache.getTile( idx ).value();
    retval = channels( currentTile , _tSpec );
    retval = scale( currentTile , _tSpec );
    return retval;
}
