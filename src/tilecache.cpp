#include "config.h"
#include "tilecache.h"

#include "util.h"
#include "smt.h"
#include "smf.h"

#include "elog/elog.h"
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <string>

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
        // LOG(INFO) << "request: " << n << " - tiles to date: " << *i
        //     << " + tiles in file: " << smt->getNTiles() << " = "
        //     << n - *i + smt->getNTiles();
        delete smt;
    }
    else if( (image = ImageInput::open( *fileName )) ){
        tileBuf =  new ImageBuf( *fileName );
        delete image;

        // Load
        tileBuf->read( 0, 0, false, TypeDesc::UINT8 );
        if(! tileBuf->initialized() ) {
            delete tileBuf;
            LOG( ERROR ) << "failed to open source for tile: " << n;
            return NULL;
        }
    }

    return tileBuf;
}



ImageBuf *
TileCache::getScaled( uint32_t n, uint32_t w, uint32_t h )
{
    ImageBuf *tileBuf = NULL;
    if(! (tileBuf = getOriginal( n )) )return NULL;
    ImageSpec spec = tileBuf->spec();

    if( h == 0 ) h = w;

    // Scale the tile to match output requirements
    ImageBuf fixBuf;
    ROI roi( 0, w, 0, h, 0, 1, 0, 4 );
    if( w != 0 || spec.width != roi.xend || spec.height != roi.yend ){
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

    return tileBuf;
}

void
TileCache::addSource( std::string fileName )
{
    ImageInput *image = NULL;
    ImageSpec spec;
    if( (image = ImageInput::open( fileName )) ){
        nTiles++;
        map.push_back( nTiles );
        fileNames.push_back( fileName );

        delete image;
        return;
    }

    SMT *smt = NULL;
    if( (smt = SMT::open( fileName )) ){
        if(! smt->getNTiles() ) return;
        nTiles += smt->getNTiles();
        map.push_back( nTiles );
        fileNames.push_back( fileName );

        delete smt;
        return;
    }

    SMF *smf = NULL;
    if( (smf = SMF::open( fileName )) ){
        // get the fileNames here
        std::vector< std::string > smtList = smf->getTileFileNames();
        for( auto i = smtList.begin(); i != smtList.end(); ++i ){
            addSource( *i );
        }
        delete smf;
        return;
    }

    LOG(ERROR) << "unrecognised format: " << fileName;
}

ImageBuf *
TileCache::operator() ( uint32_t idx )
{
    return getOriginal( idx);
}
