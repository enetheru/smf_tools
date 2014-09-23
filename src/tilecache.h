#ifndef TILECACHE_H
#define TILECACHE_H

#include <OpenImageIO/imagebuf.h>
#include <vector>
#include <string>

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
    OpenImageIO::ImageBuf* getScaled( uint32_t n, uint32_t w, uint32_t h = 0 );
};

#endif //TILECACHE_H
