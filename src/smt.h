#ifndef __SMT_H
#define __SMT_H

#include "byteorder.h"

#define SMALL_TILE_SIZE 680

/*
map texture tile file (.smt) layout is like this

TileFileHeader

Tiles
.
.
.
*/

struct TileFileHeader
{
	char magic[16] = "spring tilefile";  //"spring tilefile\0"
	int version; //must be 1 for now

	int numTiles; //total number of tiles in this file
	int tileSize = 32; //must be 32 for now
	int compressionType = 1; //must be 1=dxt1 for now
};

struct TileCRC
{
	char crc[8];
	char n = '\0';
};

#define READ_TILEFILEHEADER(tfh,src)                \
do {                                                \
	unsigned int __tmpdw;                           \
	(src).Read(&(tfh).magic,sizeof((tfh).magic));   \
	(src).Read(&__tmpdw,sizeof(unsigned int));      \
	(tfh).version = (int)swabdword(__tmpdw);        \
	(src).Read(&__tmpdw,sizeof(unsigned int));      \
	(tfh).numTiles = (int)swabdword(__tmpdw);       \
	(src).Read(&__tmpdw,sizeof(unsigned int));      \
	(tfh).tileSize = (int)swabdword(__tmpdw);       \
	(src).Read(&__tmpdw,sizeof(unsigned int));      \
	(tfh).compressionType = (int)swabdword(__tmpdw);\
} while (0)

//this is followed by the raw data for the tiles

class SMT {
public:
	TileFileHeader header;

	// Tile building
	int *tileIndex;
	vector<TileCRC> hash;

	// Sources
	vector<string> sourceImages;
	int sourceImageStride;

	// Output
	string outFileName;
	ofstream outFile;
	bool write();
};

#endif //ndef __SMT_H
