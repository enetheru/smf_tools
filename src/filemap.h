#ifndef MAPCONV_FILEMAP_H
#define MAPCONV_FILEMAP_H

#include <cstdint>
#include <string>
#include <vector>
#include <spdlog/spdlog.h>


/// simple map class for checking overlapping regions of memory
class FileMap{
    struct Block{
        uint32_t begin;
        uint32_t end;
        std::string name;
    };

    std::vector< Block > list;

public:
    void addBlock( uint32_t begin, uint32_t size, const std::string& name = "");
};

#endif //MAPCONV_FILEMAP_H
