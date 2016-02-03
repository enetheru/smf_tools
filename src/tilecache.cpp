#include <string>

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>

#include "elog/elog.h"

#include "config.h"
#include "util.h"
#include "smt.h"
#include "smf.h"
#include "tilecache.h"


OIIO_NAMESPACE_USING;

ImageBuf *
TileCache::getOriginal( uint32_t n )
{
    ImageBuf *tileBuf = nullptr;
    ImageInput *image = nullptr;
    SMT *smt = nullptr;
    static SMT *lastSmt = nullptr;
    if( n >= nTiles ) return nullptr;

    auto i = map.begin();
    auto fileName = fileNames.begin();
    while( *i <= n ) { ++i; ++fileName; }

    if( lastSmt && (! lastSmt->fileName.compare( *fileName )) ){
        tileBuf = lastSmt->getTile( n - *i + lastSmt->nTiles);
    }
    else if( (smt = SMT::open( *fileName )) ){
        // LOG(INFO) << "request: " << n << " - tiles to date: " << *i
        //     << " + tiles in file: " << smt->nTiles << " = "
        //     << n - *i + smt->nTiles;
        delete lastSmt;
        lastSmt = smt;
        tileBuf = lastSmt->getTile( n - *i + lastSmt->nTiles);
    }
    else if( (image = ImageInput::open( *fileName )) ){
        tileBuf =  new ImageBuf( *fileName );
        delete image;

        // Load
        if(! tileBuf->initialized() ) {
            delete tileBuf;
            LOG( ERROR ) << "failed to open source for tile: " << n;
            return nullptr;
        }
    }

    return tileBuf;
}

ImageBuf *
TileCache::getSpec( uint32_t n, const OpenImageIO::ImageSpec spec )
{
    CHECK( spec.width ) << "TileCache::getSpec, cannot request zero width";
    CHECK( spec.height ) << "TileCache::getSpec, cannot request zero height";
    CHECK( spec.nchannels ) << "TileCache::getSpec, cannot request zero channels";

    ImageBuf *tempBuf = nullptr;
    if(! (tempBuf = getOriginal( n )) )return nullptr;

    convert( tempBuf, spec);
    scale( tempBuf, spec);

    return tempBuf;
}

void
TileCache::addSource( std::string fileName )
{
    ImageInput *image = nullptr;
    ImageSpec spec;
    if( (image = ImageInput::open( fileName )) ){
        image->close();
        delete image;

        nTiles++;
        map.push_back( nTiles );
        fileNames.push_back( fileName );
        return;
    }

    SMT *smt = nullptr;
    if( (smt = SMT::open( fileName )) ){
        if(! smt->nTiles ) return;
        nTiles += smt->nTiles;
        map.push_back( nTiles );
        fileNames.push_back( fileName );

        delete smt;
        return;
    }

    SMF *smf = nullptr;
    if( (smf = SMF::open( fileName )) ){
        // get the fileNames here
        auto smtList = smf->getSMTList();
        for( auto i : smtList ) addSource( i.second );
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
