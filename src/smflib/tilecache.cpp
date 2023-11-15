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
OIIO::ImageBuf
TileCache::getTile( const uint32_t index ) const {
    SPDLOG_INFO("Requesting tile: {} of {}", index+1, _numTiles );
    if( index >= _numTiles ){
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

    /*auto iterator = std::find_if(sources.begin(), sources.end(),
        [index]( auto source ){
            bool result = index >= source.iStart;
            result &= index <= source.iEnd;
            if( result ){
                SPDLOG_INFO( "index: {} = Tile Source: {{ iStart: {}, iEnd: {}, filename: {}", index, source.iStart, source.iEnd, source.filePath.string() );
            }
            return result;
    } );*/
    SPDLOG_INFO( "this TileCache: ", json().dump() );
    TileSource tileSource{0,0, TileSourceType::None, "" };
    //FIXME dont loop through entire source list when we can use a find expression.
    for( const auto& source : _sources ){
        if( index >= source.iStart && index <= source.iEnd ){
            tileSource = source;
        }
    }

    if( tileSource.type == TileSourceType::None ){
        SPDLOG_ERROR("Unable to find TileSource for tile index '{}' in TileCache", index );
        return {};
    }
    SPDLOG_INFO("TileSource: {}", tileSource.json().dump() );

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

bool
TileCache::addSource( const std::filesystem::path& filePath ) {
    SPDLOG_INFO("Adding {} to TileCache", filePath.string() );
    auto image = OIIO::ImageInput::open( filePath );
    if( image ){
        SPDLOG_INFO( "{} is an image file", filePath.string() );
        image->close();
        auto start = _numTiles; _numTiles++;
        _sources.emplace_back(start, _numTiles - 1, TileSourceType::Image, filePath.string() );
        return false;
    }

    std::unique_ptr<SMT> smt( SMT::open( filePath ) );
    if( smt ){
        SPDLOG_INFO(smt->json().dump(4) );
        auto start = _numTiles; _numTiles += smt->getNumTiles();
        _sources.emplace_back(start, _numTiles - 1, TileSourceType::SMT, filePath.string() );
        return false;
    }

    std::unique_ptr<SMF> smf( SMF::open( filePath ) );
    if( smf ){
        SPDLOG_INFO( "{} is an SMF file Adding any references to SMT files", filePath.string() );
        // get the fileNames here
        auto smtList = smf->getSMTList();
        bool result = false;
        for( const auto& [numTiles,fileName] : smtList ){
            SPDLOG_INFO("Adding {} from {}", fileName, filePath.string() );
            result |= addSource( fileName );
            //auto start = _numTiles; _numTiles += numTiles;
            //sources.emplace_back(start, _numTiles-1, TileSourceType::SMT, filePath.parent_path() / fileName );
            //FIXME is it OK to recursively call or should I error if there is an error.
        }
        return result;
    }

    SPDLOG_ERROR( "Unable to open file: {}", filePath.string() );
    return true;
}

TileCache::TileCache( TileCache &&other ) noexcept
    : _numTiles( std::exchange(other._numTiles,0) ),
      _sources( std::move( other._sources ) )
    { }

TileCache::TileCache( const TileCache &other )
    : _numTiles( other._numTiles ) ,
      _sources( other._sources ) {}

TileCache &TileCache::operator=( const TileCache &other) {
    if( this == &other ) return *this;
    _numTiles = other._numTiles;
    _sources = other._sources;
    return *this;
}

TileCache &TileCache::operator=(TileCache &&other) noexcept {
    if( this == &other )return *this;
    _numTiles = std::exchange( other._numTiles, 0 );
    _sources = std::exchange( other._sources, {} );
    return *this;
}

nlohmann::ordered_json TileCache::json() const {
    nlohmann::ordered_json j;
    j["numTiles"] = _numTiles;
    j["numSources"] = _sources.size();
    j["sources"] = nlohmann::json::array();
    std::for_each(_sources.begin(), _sources.end(), [&j](const auto & source){
        j["sources"] += source.json().dump();
    });
    return j;
}
