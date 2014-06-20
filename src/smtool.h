#ifndef SMT_UTIL_H
#define SMT_UTIL_H

#include "smt.h"
#include "tilecache.h"

OIIO_NAMESPACE_USING
using namespace std;
/*
class SMTUtilbeans {
    TileCache tileCache;

    // Input Files
    string tilemapFN = "";
    string decalFN   = "";
    
public:
    int   mapWidth, mapLength;

    float cpet      = 0.0f;
    int   cnet      = 0,
          cnum      = 0,
          stride    = 1;

    void   setTilemapFN ( string s ){ tilemapFN = s; };
    void   setDecalFN   ( string s ){ decalFN   = s; };

    string getTilemapFN( ){ return tilemapFN; };
    string getDecalFN  ( ){ return decalFN;   };

    // Map
    void setWidth  ( int w ){ mapWidth  = w; };
    void setLength ( int l ){ mapLength = l; };
    void setSize   ( int s ){ mapWidth  = mapLength = s; };
    void setSize   ( int w, int l ){ mapWidth = w; mapLength = l; };

    // stitch the tiles from source images together
    ImageBuf *buildBig();

    // paste decals onto tiles.
    void pasteDecals(ImageBuf *bigBuf);    

    // === Actions output ===
///    bool save( string s ){ return save(); };
///    bool save2();

    // Collates the tiles from the tilecache into one large image.
    ImageBuf *collate( int stride ); 

}; */

class SMTool
{
public:
    bool  verbose   = false,
          quiet     = false;

    ImageBuf *reconstruct( TileCache &, string );
};

#endif //SMT_UTIL_H
