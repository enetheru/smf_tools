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
    string loadFile    = "";
    string saveFile    = "";
    string tilemapFile = "";
    string decalFile   = "";
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

    void setLoadFile(   string s ) { loadFile    = s;             };
    void setSaveFile(   string s ) { saveFile    = s;             };
    void setTilemap(    string s ) { tilemapFile = s;             };
    void setDecalFile(  string s ) { decalFile   = s;             };
    void addTileSource( string s ) { sourceFiles.push_back( s );  };

    // size is in spring map units.
    void setSize( int w, int l )    { width = w; length = l; };

    bool load();
    bool load( string s )    { loadFile = s; return load(); };

    bool save();
    bool save( string s )    { saveFile = s; return save(); };

    bool save2();

    bool append(ImageBuf &);

    void setType( int comp ); // 1=DXT1

    // pull a RGBA tile from the loaded smt file.
    ImageBuf *getTile(int tile);

    // pull the whole set of tiles, either as a collated image, or if tilemap
    // is specified as a reconstruction.
    ImageBuf *getBig();
    ImageBuf *collateBig();
    ImageBuf *reconstructBig();
    
    // Collate input images into one large buffer using stride
    ImageBuf *buildBig();

    // using decalFile, paste images onto the bigBuf
    void pasteDecals(ImageBuf *bigBuf);    

    bool decompile();
};


#endif //ndef __SMT_H
