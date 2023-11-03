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
    SPDLOG_INFO( "this TileCache: ", info() );
    TileSource tileSource{0,0, TileSourceType::None, "" };
    for( auto source : _sources ){
        SPDLOG_INFO("TileSource: {}", source.info() );
        if( index >= source.iStart && index <= source.iEnd ){
            tileSource = source;
        }
    }

    if( tileSource.type == TileSourceType::None ){
        SPDLOG_ERROR("Unable to find TileSource for tile index '{}' in TileCache", index );
        return {};
    }

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
        _sources.emplace_back(start, _numTiles - 1, TileSourceType::Image, filePath.string() );
        return;
    }

    std::unique_ptr<SMT> smt( SMT::open( filePath ) );
    if( smt ){
        SPDLOG_INFO( smt->info() );
        auto start = _numTiles; _numTiles += smt->getNumTiles();
        _sources.emplace_back(start, _numTiles - 1, TileSourceType::SMT, filePath.string() );
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

std::string TileCache::info() const {
    std::string result;
    result = std::format("{{ numTiles: {}, numSources: {}", _numTiles, _sources.size() );
    std::format_to( std::back_inserter(result), "\n\tsources {{");
    std::for_each(_sources.begin(), _sources.end(), [&result](const auto & source){
        std::format_to( std::back_inserter(result), "\n\t\t{}", source.info() );
    });
    std::format_to( std::back_inserter(result), "\n\t}}\n}}" );
    return result;
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