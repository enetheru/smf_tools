#pragma once

#include <vector>
#include <string>

#include <OpenImageIO/imagebuf.h>

class TileCache
{
    // member data
    uint32_t nTiles = 0;
    std::vector< uint32_t > map;
    std::vector< std::string > fileNames;

public:
    // modifications
    void addSource( std::string );

    // data access
    uint32_t getNTiles ( ){ return nTiles; };
    uint32_t getNFiles ( ){ return fileNames.size(); };
    OpenImageIO::ImageBuf *getOriginal( uint32_t n );
    OpenImageIO::ImageBuf *operator() ( uint32_t idx );
    OpenImageIO::ImageBuf* getSpec( uint32_t n, const OpenImageIO::ImageSpec );
};
