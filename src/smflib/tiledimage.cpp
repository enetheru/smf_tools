#include <utility>

#include <spdlog/spdlog.h>

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>

#include "tiledimage.h"
#include "util.h"

// CONSTRUCTORS
// ============
TiledImage::TiledImage(const OIIO::ImageSpec& imageSpec, uint32_t tileWidth, uint32_t tileHeight, uint32_t tileOverlap )
    : _tileWidth( tileWidth ), _tileHeight( tileHeight ), _tileOverlap( tileOverlap ), _imageSpec( imageSpec ) {
    _tileMap.setSize( imageSpec.width / (tileWidth - tileOverlap), imageSpec.height / (tileHeight - tileOverlap) );
}

TiledImage::TiledImage( TileCache tileCache, const TileMap& tileMap ):
        _tileCache( std::move( tileCache ) ), _tileMap( tileMap ) {
    SPDLOG_INFO("Constructing TiledImage from TileCache and TileMap");
    SPDLOG_INFO( "TileCache: {}", _tileCache.json().dump(4) );
    SPDLOG_INFO( "TileMap: {}", _tileMap.json().dump(4) );
    // How to get the tile size, and the imageSpec from these?
    auto imageBuf = _tileCache.getTile(1);
    _imageSpec = imageBuf.spec();
    _tileWidth = _imageSpec.width;
    _tileHeight = _imageSpec.height;
}

// ACCESS
// ======
OIIO::ImageBuf
TiledImage::getRegion( const OIIO::ROI &roi ) {
    SPDLOG_INFO( "source window ({}, {})->({}, {})", roi.xbegin, roi.ybegin, roi.xend, roi.yend );

    OIIO::ImageSpec outSpec( _imageSpec );
    outSpec.width = roi.width();
    outSpec.height = roi.height();

    OIIO::ImageSpec tileSpec( _imageSpec );
    tileSpec.width = _tileWidth;
    tileSpec.height = _tileHeight;

    OIIO::ImageBuf outBuf( outSpec );
    //outBuf->write( "TiledImage_getRegion_outBuf.tif", "tif" );

    //current point of interest
    uint32_t ix = roi.xbegin;
    uint32_t iy = roi.ybegin;
    static uint32_t index_p = INT_MAX;
    OIIO::ROI copyRegion{0, 0, 0, 0, 0, 1, 0, 4};
    OIIO::ImageBuf currentTile;
    while( true ){
         SPDLOG_INFO( "Point of interest ({}, {})", ix, iy );

        //determine the tile under the point of interest
        uint32_t mx = ix / (_tileWidth - _tileOverlap);
        uint32_t my = iy / (_tileHeight - _tileOverlap);

        //determine the top left corner of the copy window
        copyRegion.xbegin = ix - mx * (_tileWidth - _tileOverlap);
        copyRegion.ybegin = iy - my * (_tileHeight - _tileOverlap);

        //determine the bottom right corner of the copy window
        if( roi.xend / (_tileWidth - _tileOverlap) > mx ) copyRegion.xend = _tileWidth;
        else copyRegion.xend = roi.xend - mx * (_tileWidth - _tileOverlap);

        if( roi.yend / (_tileHeight - _tileOverlap) > my ) copyRegion.yend = _tileHeight;
        else copyRegion.yend = roi.yend - my * (_tileHeight - _tileOverlap);

        SPDLOG_INFO("copy window ({}, {})->({}, {})", copyRegion.xbegin, copyRegion.ybegin, copyRegion.xend, copyRegion.yend );

        //determine the dimensions of the copy window
        SPDLOG_INFO("copy window size {}x{}", copyRegion.width(), copyRegion.height() );

        //determine the top left of the paste window
        uint32_t dx = ix - roi.xbegin;
        uint32_t dy = iy - roi.ybegin;
        SPDLOG_INFO( "Paste position: {}x{}", dx, dy );

        //Optimisation: exact copy of previous tile test
        uint32_t index = _tileMap.getXY(mx, my);
        if( index != index_p ){
            // create blank tile if index is out of range
            if(index >= _tileCache.getNumTiles() ){
                currentTile.reset( tileSpec );
            } else {
                currentTile = _tileCache.getTile(index);
                // A possibility exists that the tile cache will give us a tile that
                // has dimensions other than expected. So let's check and scale.
                //currentTile = fix_scale( std::move( currentTile ), _tSpec );
                currentTile = scale( currentTile, tileSpec );
                // it's possible that the tile retrieved has different channels
                // than the tiledImage spec
                currentTile = channels( currentTile, tileSpec );
            }
            index_p = index;
        }
        if( currentTile.initialized() ){
            //copy pixel data from source tile to dest
            OIIO::ImageBufAlgo::paste(outBuf, dx, dy, 0, 0, currentTile, copyRegion );
            //outBuf->write( "TiledImage_getRegion_outBuf_paste.tif", "tif");
        }

        //determine the next point of interest
        ix += copyRegion.width();
        if( int( ix ) >= roi.xend ){
            ix = roi.xbegin;
            iy += copyRegion.height();
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
TiledImage::getUVRegion( OIIO::ROI roi ) {
    int width = _tileMap.width() * _tileWidth;
    int height = _tileMap.height() * _tileHeight;
    roi.xbegin *= width;
    roi.xend *= width;
    roi.ybegin *= height;
    roi.yend *= height;
    return getRegion( roi );
}

void TiledImage::setImageSpec( const OIIO::ImageSpec& imageSpec ) {
    // The width and height of the imagespec is always derived from the tilemap and the tilesize.
    _imageSpec = imageSpec;
}

void TiledImage::setTileMap(const TileMap &tileMap) {
    _tileMap = tileMap;
}

void
TiledImage::setTileSize( uint32_t width, uint32_t height, uint32_t overlap ) {
    _tileWidth = width;
    _tileHeight = height;
    _tileOverlap = overlap;
}


