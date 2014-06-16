#ifndef __SMT_H
#define __SMT_H

#include <OpenImageIO/imagebuf.h>

#define DXT1 1

OIIO_NAMESPACE_USING
using namespace std;

/*
map texture tile file (.smt) layout is like this

TileFileHeader

Tiles
.
.
.
*/

struct SMTHeader
{
    char magic[16] = "spring tilefile";   // "spring tilefile\0"
    int version = 1;      // must be 1 for now
    int nTiles = 0;       // total number of tiles in this file
    int tileRes = 32;      // x and y dimension of tiles, must remain 32 for now.
    int tileType = DXT1;     // must be 1=dxt1 for now
};

#define READ_SMTHEADER(tfh,src)                     \
do {                                                \
    unsigned int __tmpdw;                           \
    (src).Read(&(tfh).magic,sizeof((tfh).magic));   \
    (src).Read(&__tmpdw,sizeof(unsigned int));      \
    (tfh).version = (int)swabdword(__tmpdw);        \
    (src).Read(&__tmpdw,sizeof(unsigned int));      \
    (tfh).count = (int)swabdword(__tmpdw);          \
    (src).Read(&__tmpdw,sizeof(unsigned int));      \
    (tfh).tileRes = (int)swabdword(__tmpdw);        \
    (src).Read(&__tmpdw,sizeof(unsigned int));      \
    (tfh).comp = (int)swabdword(__tmpdw);\
} while (0)

//this is followed by the raw data for the tiles

class SMT {
    SMTHeader header;
    int   width     = 8,
          length    = 8;

    // Tiles
    int   nTiles    = 0,
          tileType  = 1,
          tileRes   = 32,
          tileSize;

    // Input Files
    string loadFN    = "";
    string saveFN    = "";
    string tilemapFN = "";
    string decalFN   = "";
    vector<string> sourceFiles;
    
public:
    bool  verbose   = true,
          quiet     = false,
          slow_dxt1 = false;
    float cpet      = 0.0f;
    int   cnet      = 0,
          cnum      = 0,
          stride    = 1;

    SMT() { setType( DXT1 ); };
    SMT( string file ) { load( file ); };

    void   setLoadFN    ( string s ){ loadFN    = s; };
    void   setSaveFN    ( string s ){ saveFN    = s; };
    void   setTilemapFN ( string s ){ tilemapFN = s; };
    void   setDecalFN   ( string s ){ decalFN   = s; };

    string getLoadFN   ( ){ return loadFN;    };
    string getSaveFN   ( ){ return saveFN;    };
    string getTilemapFN( ){ return tilemapFN; };
    string getDecalFN  ( ){ return decalFN;   };

    // Map
    void setWidth  ( int w ){ width  = w; };
    void setLength ( int l ){ length = l; };
    void setSize   ( int s ){ width  = length = s; };
    void setSize   ( int w, int l ){ width = w; length = l; };

    // Tile
    void addTileSource( string s ){ sourceFiles.push_back( s ); };
    void setTileRes( int s    ){ tileRes = s; setType(tileType); };
    void setType   ( int comp ); // 1=DXT1

    int getTileType( ){ return tileType; };
    int getTileRes ( ){ return tileRes;  };
    int getTileSize( ){ return tileSize; };

    // === Actions input ===
    bool load();
    bool load( string s ){ loadFN = s; return load(); };
    ImageBuf *getTile(int tile);

    // === Actions modify ===
    // stitch the tiles loaded from the smt together
    ImageBuf *getBig(); // calls either collate or reconstruct
    ImageBuf *collateBig();
    ImageBuf *reconstructBig();

    // stitch the tiles from source images together
    ImageBuf *buildBig();

    // paste decals onto tiles.
    void pasteDecals(ImageBuf *bigBuf);    

    // === Actions output ===
    bool save();
    bool save( string s ){ saveFN = s; return save(); };

    bool save2();

    bool append(ImageBuf &);

    bool decompile();
};


#endif //ndef __SMT_H
