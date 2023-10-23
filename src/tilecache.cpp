#include <string>
#include <OpenImageIO/imagebuf.h>
#include <spdlog/spdlog.h>

#include "smf_tools.h"
#ifdef IMG_DEBUG
#include <sstream>
#endif

#include "smt.h"
#include "smf.h"
#include "tilecache.h"

std::unique_ptr< OIIO::ImageBuf >
//FIXME review this function for memory leaks
TileCache::getTile(const uint32_t n)
{
    std::unique_ptr< OIIO::ImageBuf >
        outBuf( new OIIO::ImageBuf );

    // returning an initialized imagebuf is not a good idea
    if( n >= nTiles ){
        spdlog::critical( "getTile({}) request out of range 0-{}", n, nTiles );
        return nullptr;
    }

    SMT *smt;
    static SMT *lastSmt = nullptr;

    // FIXME, what the fuck does this do?
    auto i = map.begin();
    auto fileName = fileNames.begin();
    while( *i <= n ){
        ++i;
        ++fileName;
    }

    // already open smt file?
    if( lastSmt && (! lastSmt->filePath.compare( *fileName )) ){
        outBuf = lastSmt->getTile( n - *i + lastSmt->nTiles);
    }
    // open a new smt file?
    else if  ( (smt = SMT::open( *fileName )) ){
        //FIXME shouldn't have a manual delete here, use move semantics instead
        delete lastSmt;
        lastSmt = smt;
        outBuf = lastSmt->getTile( n - *i + lastSmt->nTiles );
    }
    // open the image file?
    else {
        outBuf->reset( fileName->string() );
    }
    if( !outBuf->initialized() ) {
        spdlog::error("failed to open source for tile: ", n);
        return nullptr;
    }

#ifdef DEBUG_IMG
    DLOG( INFO ) << "Exporting Image";
    outBuf->write( "TileCache.getTile.tif" , "tif" );
#endif

    return outBuf;
}

//TODO go over this function to see if it can be refactored
void
TileCache::addSource( std::filesystem::path filePath ) {
    auto image = OIIO::ImageInput::open( filePath );
    if( image ){
        image->close();

        _nTiles++;
        map.push_back( nTiles );
        fileNames.push_back( filePath.filename() );
        return;
    }

    SMT *smt = SMT::open( filePath );
    if( smt ){
        if(! smt->nTiles ) return;
        _nTiles += smt->nTiles;
        map.push_back( nTiles );
        fileNames.push_back( filePath.filename() );

        delete smt;
        return;
    }

    SMF *smf = SMF::open( filePath );
    if( smf ){
        // get the fileNames here
        auto smtList = smf->getSMTList();
        for( const auto& [a,b] : smtList ) addSource( b );
        delete smf;
        return;
    }

    spdlog::error( "unrecognised format: ", filePath.string() );
}


