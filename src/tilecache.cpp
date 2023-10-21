#include <string>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <elog.h>

#include "smf_tools.h"
#ifdef IMG_DEBUG
#include <sstream>
#endif

#include "util.h"
#include "smt.h"
#include "smf.h"
#include "tilecache.h"

std::unique_ptr< OIIO::ImageBuf >
//FIXME remove the 2 once all is said and done
TileCache::getTile(const uint32_t n)
{
    std::unique_ptr< OIIO::ImageBuf >
        outBuf( new OIIO::ImageBuf );

    // returning an unitialized imagebuf is not a good idea
    CHECK( n < nTiles ) << "getTile( " << n << ") request out of range 0-" << nTiles ;

    SMT *smt = nullptr;
    static SMT *lastSmt = nullptr;

    // FIXME, what the fuck does this do?
    auto i = map.begin();
    auto fileName = fileNames.begin();
    while( *i <= n ){
        ++i;
        ++fileName;
    }

    // already open smt file?
    if( lastSmt && (! lastSmt->fileName.compare( *fileName )) ){
        outBuf = lastSmt->getTile( n - *i + lastSmt->nTiles);
    }
    // open a new smt file?
    else if  ( (smt = SMT::open( *fileName )) ){
        //FIXME shouldnt have a manual delete here, use move scemantics instead
        delete lastSmt;
        lastSmt = smt;
        outBuf = lastSmt->getTile( n - *i + lastSmt->nTiles );
    }
    // open the image file?
    else {
        outBuf->reset( *fileName );
    }
    CHECK( outBuf->initialized() ) << "failed to open source for tile: " << n;

#ifdef DEBUG_IMG
    DLOG( INFO ) << "Exporting Image";
    outBuf->write( "TileCache.getTile.tif" , "tif" );
#endif

    return outBuf;
}

//TODO go over this function to see if it can be refactored
void
TileCache::addSource( const std::string fileName )
{
    OIIO_NAMESPACE_USING;
    auto image = ImageInput::open( fileName );
    if( image ){
        image->close();

        _nTiles++;
        map.push_back( nTiles );
        fileNames.push_back( fileName );
        return;
    }

    SMT *smt = nullptr;
    if( (smt = SMT::open( fileName )) ){
        if(! smt->nTiles ) return;
        _nTiles += smt->nTiles;
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


