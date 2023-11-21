#include "filemap.h"

#include <spdlog/spdlog.h>

bool FileMap::addBlock( const std::streampos begin, const std::streamsize size, const std::string& name ) {
    SPDLOG_DEBUG( "Adding DataBlock:{{ name: {}, begin: {}, size: {} }}",
                  name, static_cast< int >(begin), static_cast< int >(size) );

    DataBlock newBlock{ begin, begin + (size - 1), name };

    bool clash = false;
    for( const auto& block : dataBlocks ) {
        if( (newBlock.begin >= block.begin && newBlock.begin <= block.end)
            || (newBlock.end >= block.begin && newBlock.end <= block.end)
            || (newBlock.begin <= block.begin && newBlock.end >= block.end) ) {
            clash = true;
            overlappingBlocks.emplace_back( newBlock, block );
            SPDLOG_ERROR( "'{}' clashes with existing block '{}'", newBlock.name, block.name );
        }
    }
    dataBlocks.push_back( newBlock );
    return clash;
}

std::optional< FileMap::DataBlock > FileMap::getBlockAt( const std::streampos position ) const {
    std::optional< DataBlock > match;
    for( const auto& block : dataBlocks ) {
        if( block.begin <= position && block.end > position ) match = block;
    }
    return match;
}
