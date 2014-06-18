#ifndef TILECACHE_H
#define TILECACHE_H

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>
#include <vector>
#include <string>

OIIO_NAMESPACE_USING
using namespace std;

class TileCache
{
    int nTiles = 0;
    vector<int> map;
    vector<string> filenames; 
public:
    void push_back( string );
    int getNTiles( ){ return nTiles; };
    ImageBuf* getTile( int );
};

#endif //TILECACHE_H
