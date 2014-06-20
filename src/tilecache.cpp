#include "tilecache.h"
#include "util.h"
#include "smt.h"

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>

using namespace std;
OIIO_NAMESPACE_USING;


ImageBuf *
TileCache::getTile( unsigned int n )
{
    ImageBuf *tileBuf = NULL;
    ImageInput *image = NULL;
    SMT *smt = NULL;
    if( n > nTiles ) return NULL;

    unsigned int i = 0;
    while( map[ i ] < n ) ++i;

    if( (smt = SMT::open( filenames[ i ] )) ){
        tileBuf = smt->getTile( map[ i ] - n );
        delete smt;
    }
    else if( (image = ImageInput::open( filenames[ i ] )) ){
        tileBuf =  new ImageBuf( filenames[ i ] );
        delete image;
    }

    // Load
    tileBuf->read( 0, 0, false, TypeDesc::UINT8 );
    if(! tileBuf->initialized() ) {
        return NULL;
    }
    ImageSpec spec = tileBuf->spec();
  

    // Scale the tile to match output requirements
    ImageBuf fixBuf; 
    ROI roi( 0, tileRes, 0, tileRes, 0, 1, 0, 4 );
    if( spec.width != roi.xend || spec.height != roi.yend ) {
//            printf( "WARNING: Image is (%i,%i), wanted (%i, %i),"
//                " Resampling.\n",
//                spec.width, spec.height, roi.xend, roi.yend );
 
        ImageBufAlgo::resample( fixBuf, *tileBuf, false, roi );
        tileBuf->copy( fixBuf );
        fixBuf.clear();
    }

    // Add alpha channel if it doesnt exist
    if( spec.nchannels < 4 )
    {
        int map[] = { 0, 1, 2, -1 };
        float fill[] = { 0, 0, 0, 255 };
        ImageBufAlgo::channels( fixBuf, *tileBuf, 4, map, fill );
        tileBuf->copy( fixBuf );
        fixBuf.clear();
    }


#ifdef DEBUG_IMG
    tileBuf->save("TileCache::getTile(" + to_string(n) + ").tif", "tif");
#endif //DEBUG_IMG

    return tileBuf;
}

void
TileCache::push_back( string fileName )
{
    ImageInput *image;
    ImageSpec spec;
    if( (image = ImageInput::open( fileName )) ){
        nTiles++;
        map.push_back( nTiles );
        filenames.push_back( fileName );

        if(! tileRes ){
            spec = image->spec();
            tileRes = fmin( spec.width, spec.height );
        }
        delete image;
        return;
    }

    SMT *smt = NULL;
    if( (smt = SMT::open( fileName )) ){
        nTiles += smt->getNTiles();
        map.push_back( nTiles );
        filenames.push_back( fileName );

        if(! tileRes ){
            tileRes = smt->getTileRes();
        }
        delete smt;
        return;
    }
}
