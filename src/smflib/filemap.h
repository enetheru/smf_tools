#ifndef MAPCONV_FILEMAP_H
#define MAPCONV_FILEMAP_H

#include <optional>
#include <string>
#include <vector>

/// simple map class for checking overlapping regions of memory
class FileMap {
    struct DataBlock {
        std::streampos begin{};
        std::streampos end{};
        std::string name{};
    };

    std::vector< DataBlock > dataBlocks;
    std::vector< std::pair< DataBlock, DataBlock > > overlappingBlocks;

public:
    bool addBlock( std::streampos begin, std::streamsize size, const std::string& name = "" );
    [[nodiscard]] std::optional< DataBlock > getBlockAt( std::streampos position ) const;
    //TODO add bool hasOverlap()
    //TODO add bool removeBlock(...)
};

#endif //MAPCONV_FILEMAP_H
