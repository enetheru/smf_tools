#include "filemap.h"

bool FileMap::addBlock( uint32_t begin, uint32_t size, const std::string& name )
{
    SPDLOG_DEBUG( "Adding DataBlock:{{ name: {}, begin: {}, end: {} }}", name, begin, begin + size-1);
    DataBlock newBlock{begin, begin + size - 1, name };
    bool clash = false;
    for( const auto& block : dataBlocks ){
        if( (newBlock.begin >= block.begin && newBlock.begin <= block.end)
            || (newBlock.end >= block.begin && newBlock.end <= block.end)
            || (newBlock.begin <= block.begin && newBlock.end >= block.end) ){
            clash = true;
            overlappingBlocks.push_back({newBlock, block });
            SPDLOG_ERROR("'{}' clashes with existing block '{}'", newBlock.name, block.name );
        }
    }
    dataBlocks.push_back( newBlock );
    return clash;
}