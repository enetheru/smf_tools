#include "tilecache.h"

#include "util.h"
#include "smt.h"
#include "smf.h"

#include <string>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>

OIIO_NAMESPACE_USING;

ImageBuf *
TileCache::getOriginal( uint32_t n )
{
    ImageBuf *tileBuf = NULL;
    ImageInput *image = NULL;
    SMT *smt = NULL;
    if( n > nTiles ) return NULL;

    auto i = map.begin();
    auto fileName = fileNames.begin();
    while( *i <= n ) { ++i; ++fileName; }

    if( (smt = SMT::open( *fileName )) ){
        tileBuf = smt->getTile( n - *i + smt->getNTiles() );
//        cout << "request: " << n << " - tiles to date: " << *i << " + tiles in file: " << smt->getNTiles() << " = " << n - *i + smt->getNTiles() << std::endl;
        delete smt;
    }
    else if( (image = ImageInput::open( *fileName )) ){
        tileBuf =  new ImageBuf( *fileName );
        delete image;

        // Load
        tileBuf->read( 0, 0, false, TypeDesc::UINT8 );
        if(! tileBuf->initialized() ) {
            delete tileBuf;
            return NULL;
        }
    }

#ifdef DEBUG_IMG
    tileBuf->save("TileCache::getOriginal(" + std::to_string(n) + ").tif", "tif");
#endif //DEBUG_IMG

    return tileBuf;
}



ImageBuf *
TileCache::getTile( uint32_t n )
{
    ImageBuf *tileBuf = NULL;
    if(! (tileBuf = getOriginal( n )) )return NULL;
    ImageSpec spec = tileBuf->spec();

    // Scale the tile to match output requirements
    ImageBuf fixBuf; 
    ROI roi( 0, tileSize, 0, tileSize, 0, 1, 0, 4 );
    if( spec.width != roi.xend || spec.height != roi.yend ){
//            printf( "WARNING: Image is (%i,%i), wanted (%i, %i),"
//                " Resampling.\n",
//                spec.width, spec.height, roi.xend, roi.yend );
 
        ImageBufAlgo::resample( fixBuf, *tileBuf, false, roi );
        tileBuf->copy( fixBuf );
        fixBuf.clear();
    }

    // Add alpha channel if it doesnt exist
    if( spec.nchannels < 4 ){
        int map[] = { 0, 1, 2, -1 };
        float fill[] = { 0, 0, 0, 255 };
        ImageBufAlgo::channels( fixBuf, *tileBuf, 4, map, fill );
        tileBuf->copy( fixBuf );
        fixBuf.clear();
    }

#ifdef DEBUG_IMG
    tileBuf->save("TileCache::getTile(" + std::to_string(n) + ").tif", "tif");
#endif //DEBUG_IMG

    return tileBuf;
}

void
TileCache::push_back( std::string fileName )
{
    ImageInput *image;
    ImageSpec spec;
    if( (image = ImageInput::open( fileName )) ){
        nTiles++;
        map.push_back( nTiles );
        fileNames.push_back( fileName );

        if(! tileSize ){
            spec = image->spec();
            tileSize = fmin( spec.width, spec.height );
        }
        delete image;
        return;
    }

    SMT *smt = NULL;
    if( (smt = SMT::open( fileName )) ){
        if(! smt->getNTiles() ) return;
        nTiles += smt->getNTiles();
        map.push_back( nTiles );
        fileNames.push_back( fileName );

        if(! tileSize ){
            tileSize = smt->getTileSize();
        }
        delete smt;
        return;
    }

    SMF *smf = NULL;
    if( (smf = SMF::open( fileName )) ){
        // get the fileNames here
        std::vector< std::string > smtList = smf->getTileFileNames();
        for( auto i = smtList.begin(); i != smtList.end(); ++i ){
            push_back( *i );
        }
        delete smf;
        return;
    }

    if( verbose ) cout << "WARN.TileCache.push_back: unrecognised format: "
        << fileName << std::endl;
}
