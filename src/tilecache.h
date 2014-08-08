#ifndef TILECACHE_H
#define TILECACHE_H
#include "config.h"

#include <OpenImageIO/imagebuf.h>
#include <vector>
#include <string>

class TileCache
{
    uint32_t nTiles;
    uint32_t tileSize;
    std::vector< uint32_t > map;
    std::vector< std::string > fileNames; 
public:
    bool verbose, quiet;

    TileCache( bool v = false, bool q = false )
        : verbose(v), quiet(q)
    { };

    void push_back( std::string );
    void setTileSize( uint32_t t ){ tileSize = t; };

    uint32_t getNTiles ( ){ return nTiles; };
    uint32_t getNFiles ( ){ return fileNames.size(); };
    uint32_t getTileSize( ){ return tileSize; };
    OpenImageIO::ImageBuf* getTile( uint32_t );
    OpenImageIO::ImageBuf* getOriginal( uint32_t );
};

#endif //TILECACHE_H
