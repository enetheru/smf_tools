#include <OpenImageIO/imagebuf.h>
#include <spdlog/spdlog.h>
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include "smt.h"
#include "smf.h"
#include "tilecache.h"

std::string to_string( TileSourceType type ){
    switch(type){
        case TileSourceType::Image:
            return "Image";
        case TileSourceType::SMT:
            return "SMT";
        case TileSourceType::SMF:
            return "SMF";
        default:
            return "Unknown";
    }
}

//FIXME review this function for memory leaks
std::optional<OIIO::ImageBuf>
TileCache::getTile( const uint32_t index ){
    SPDLOG_INFO("Requesting tile: {} of {}", index+1, _numTiles );
    if( index+1 > _numTiles ){
        SPDLOG_CRITICAL("Request for tile {} is greater than _numTiles:{}", index+1, _numTiles );
        return {};
    }

    //FIXME pull from already loaded file.
    // already open smt file?
    /*SMT *smt;
    static SMT *lastSmt = nullptr;
    if( lastSmt && (! lastSmt->filePath.compare( *fileName )) ){
        outBuf = lastSmt->getTile( index - *i + lastSmt->nTiles);
    }*/

    auto iterator = std::find_if(sources.begin(), sources.end(),
        [index]( auto source ){ return index >= source.iStart && index <= source.iEnd; } );

    if( iterator == sources.end() ){
        SPDLOG_ERROR("Unable to find TileSource for tile index '{}' in TileCache", index );
        return {};
    }
    const auto &tileSource = *iterator;

    SPDLOG_INFO("TileSource: {{filePath: {}, type: {}, iStart: {}, iEnd: {}}}", tileSource.filePath.string(), to_string(tileSource.type), tileSource.iStart, tileSource.iEnd );

    switch( tileSource.type ){
        case TileSourceType::SMT: {
            auto smt = std::unique_ptr<SMT>( SMT::open(  tileSource.filePath ) );
            return smt->getTile( index - tileSource.iStart );
        }
        case TileSourceType::SMF: {
            //TODO evaluate if loading tiles from an SMF is viable, or should be removed.
            SPDLOG_CRITICAL("Loading tiles from an SMF is not currently implemented");
        } break;
        case TileSourceType::Image: {
            return OIIO::ImageBuf( tileSource.filePath.string() );
        }
        default:{
            SPDLOG_ERROR("TileSource has unknown type '{}' at index {}", to_string(tileSource.type), index);
            return {};
        }
    }
    return {};
}

//TODO go over this function to see if it can be refactored
void
TileCache::addSource( const std::filesystem::path& filePath ) {
    SPDLOG_INFO("Adding {} to TileCache", filePath.string() );
    auto image = OIIO::ImageInput::open( filePath );
    if( image ){
        SPDLOG_INFO( "{} is an image file", filePath.string() );
        image->close();
        auto start = _numTiles; _numTiles++;
        sources.emplace_back(start, _numTiles-1, TileSourceType::Image, filePath.string() );
        return;
    }

    std::unique_ptr<SMT> smt( SMT::open( filePath ) );
    if( smt ){
        SPDLOG_INFO( "{} is an SMT file", filePath.string() );
        auto start = _numTiles; _numTiles += smt->getNumTiles();
        sources.emplace_back(start, _numTiles-1, TileSourceType::SMT, filePath.string() );
        return;
    }

    std::unique_ptr<SMF> smf( SMF::open( filePath ) );
    if( smf ){
        SPDLOG_INFO( "{} is an SMF file Adding any references to SMT files", filePath.string() );
        // get the fileNames here
        auto smtList = smf->getSMTList();
        for( const auto& [numTiles,fileName] : smtList ){
            SPDLOG_INFO("Adding {} from {}", fileName, filePath.string() );
            addSource( fileName );
            //auto start = _numTiles; _numTiles += numTiles;
            //sources.emplace_back(start, _numTiles-1, TileSourceType::SMT, filePath.parent_path() / fileName );
        }
        return;
    }

    SPDLOG_ERROR( "Unable to open file: {}", filePath.string() );
}


