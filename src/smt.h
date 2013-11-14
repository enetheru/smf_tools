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
	SMTHeader();
	char magic[16];   //"spring tilefile\0"
	int version;      //must be 1 for now
	int nTiles;        //total number of tiles in this file
	int tileRes;     //must be 32 for now
	int tileType;     //must be 1=dxt1 for now
};

#define READ_SMTHEADER(tfh,src)                \
do {                                                \
	unsigned int __tmpdw;                           \
	(src).Read(&(tfh).magic,sizeof((tfh).magic));   \
	(src).Read(&__tmpdw,sizeof(unsigned int));      \
	(tfh).version = (int)swabdword(__tmpdw);        \
	(src).Read(&__tmpdw,sizeof(unsigned int));      \
	(tfh).count = (int)swabdword(__tmpdw);       \
	(src).Read(&__tmpdw,sizeof(unsigned int));      \
	(tfh).tileRes = (int)swabdword(__tmpdw);       \
	(src).Read(&__tmpdw,sizeof(unsigned int));      \
	(tfh).comp = (int)swabdword(__tmpdw);\
} while (0)

//this is followed by the raw data for the tiles

class SMT {
	// Loading
	SMTHeader header;
	string loadFile;

	// Saving
	string outPrefix;
	int nTiles;
	int tileRes;
	int tileType;

	// Construction
	int width, length;
	vector<string> sourceFiles;
	int tileSize;
	string decalFile;

	// Reconstruction
	string tilemapFile;
	
public:
	bool verbose, quiet, slow_dxt1;
	int stride;
	float cpet;
	int cnet, cnum;
	SMT();

	bool load(string fileName);
	bool save();

	void setPrefix(string prefix);
	void setTilemap(string filename);
	void setDecalFile(string filename);
	void setType(int comp); // 1=DXT1
	void addTileSource(string filename);
	void setSize(int w, int l); // in spring map units.

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
