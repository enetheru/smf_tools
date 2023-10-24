#include <OpenImageIO/imagebuf.h>
#include <spdlog/spdlog.h>

#include "smf_tools.h"
#ifdef IMG_DEBUG
#include <sstream>
#endif

#include "smt.h"
#include "smf.h"
#include "tilecache.h"

//FIXME review this function for memory leaks
std::optional<OIIO::ImageBuf>
TileCache::getTile( const uint32_t index ){
    // returning an initialized imagebuf is not a good idea
    if( index >= tileCount ){
        spdlog::critical( "getTile({}) request out of range 0-{}", index, tileCount );
        return {};
    }

    //FIXME pull from already loaded file.
    // already open smt file?
    /*SMT *smt;
    static SMT *lastSmt = nullptr;
    if( lastSmt && (! lastSmt->filePath.compare( *fileName )) ){
        outBuf = lastSmt->getTile( index - *i + lastSmt->nTiles);
    }*/

    const auto &tileSource = *std::find_if(sources.begin(), sources.end(),
        [index]( auto source ){ return index > source.iStart && index < source.iEnd; } );

    switch( tileSource.type ){
        case TileSourceType::SMT: {
            auto smt = std::unique_ptr<SMT>( SMT::open(  tileSource.filePath ) );
            return smt->getTile( index - tileSource.iStart + smt->getNumTiles() );
        }
        case TileSourceType::SMF: {

        } break;
        case TileSourceType::Image: {
            return OIIO::ImageBuf( tileSource.filePath.string() );
        }
        default:{
            spdlog::error("failed to open source for tile: ", index);
            return {};
        }
    }
    return {};
}

//TODO go over this function to see if it can be refactored
void
TileCache::addSource( const std::filesystem::path& filePath ) {
    auto image = OIIO::ImageInput::open( filePath );
    if( image ){
        image->close();
        sources.emplace_back(tileCount, (++tileCount), TileSourceType::Image, filePath.filename() );
        return;
    }

    std::unique_ptr<SMT> smt( SMT::open( filePath ) );
    if( smt ){
        sources.emplace_back(tileCount, (tileCount += smt->getNumTiles()), TileSourceType::SMT, filePath.filename() );
        return;
    }

    std::unique_ptr<SMF> smf( SMF::open( filePath ) );
    if( smf ){
        // get the fileNames here
        auto smtList = smf->getSMTList();
        for( const auto& [numTiles,fileName] : smtList ){
            sources.emplace_back(tileCount, (tileCount += numTiles), TileSourceType::SMF, fileName );
        }
        return;
    }

    spdlog::error( "unrecognised format: ", filePath.string() );
}


