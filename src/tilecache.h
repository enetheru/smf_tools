#ifndef TILECACHE_H
#define TILECACHE_H
#include "config.h"

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>
#include <vector>
#include <string>

OIIO_NAMESPACE_USING
using namespace std;

class TileCache
{
    unsigned int nTiles = 0;
    unsigned int tileRes = 0;
    vector<unsigned int> map;
    vector<string> filenames; 
public:
    bool verbose, quiet;

    TileCache( bool v = false, bool q = false )
        : verbose(v), quiet(q) { };

    void push_back( string );
    void setTileRes( unsigned int t ){ tileRes = t; };

    unsigned int getNTiles ( ){ return nTiles; };
    unsigned int getNFiles ( ){ return filenames.size(); };
    unsigned int getTileRes( ){ return tileRes; };
    ImageBuf* getTile( unsigned int );
    ImageBuf* getOriginal( unsigned int );
};

#endif //TILECACHE_H
