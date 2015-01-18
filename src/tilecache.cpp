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
    if( n >= nTiles ) return NULL;

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
    CHECK( w ) << "TileCache::getScaled, cannot input zero width";
    if( h == 0 ) h = w;

    ImageBuf *tempBuf = NULL;
    if(! (tempBuf = getOriginal( n )) )return NULL;

    ImageSpec spec;
    spec.width = w;
    spec.height = h;
    spec.nchannels = 4;

    scale( tempBuf, spec );
    channels( tempBuf, spec );

    return tempBuf;
}

void
TileCache::addSource( std::string fileName )
{
    ImageInput *image = NULL;
    ImageSpec spec;
    if( (image = ImageInput::open( fileName )) ){
        image->close();
        delete image;

        nTiles++;
        map.push_back( nTiles );
        fileNames.push_back( fileName );
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
