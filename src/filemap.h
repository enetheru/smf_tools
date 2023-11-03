#ifndef MAPCONV_FILEMAP_H
#define MAPCONV_FILEMAP_H

#include <cstdint>
#include <string>
#include <vector>
#include <spdlog/spdlog.h>

/// simple map class for checking overlapping regions of memory
class FileMap{
    struct DataBlock{
        uint32_t begin;
        uint32_t end;
        std::string name;
    };
    std::vector< DataBlock > dataBlocks;
    std::vector<std::pair<DataBlock, DataBlock>> overlappingBlocks;

public:
    bool addBlock( uint32_t begin, uint32_t size, const std::string& name = "" ); //TODO add[[nodiscard]] to this function
    //TODO add bool hasOverlap()
    //TODO add bool removeBlock(...)
};

#endif //MAPCONV_FILEMAP_H
