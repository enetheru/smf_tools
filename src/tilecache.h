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
    vector<unsigned int> map;
    vector<string> filenames; 
public:
    bool verbose, quiet;
    unsigned int tileSize = 32;

    TileCache( bool v = false, bool q = false ) : verbose(v), quiet(q) { };


    void push_back( string );
    unsigned int getNTiles( ){ return nTiles; };
    ImageBuf* getTile( unsigned int );
};

#endif //TILECACHE_H
