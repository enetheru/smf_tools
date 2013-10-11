#ifndef __TILEFILE_H
#define __TILEFILE_H

#include "byteorder.h"

// defined by DXT1 compression of 32x32 pixel tile with 4 mip levels
// Would not hold true if other compression types were made available
// 32x32, 16x16, 8x8, 4x4
// 512  + 128  + 32 + 8
#define SMALL_TILE_SIZE 680

struct tile_crc {
	char d[8];
	char n;
	tile_crc();
};

tile_crc::tile_crc()
{
	n = '\0';
}

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
	char magic[16];  //"spring tilefile\0"
	int version; //must be 1 for now
	int tiles; //total number of tiles in this file
	int size; //must be 32 for now
	int comp; //must be 1=dxt1 for now
};

#define READ_SMTHEADER(tfh,src)                     \
do {                                                \
	unsigned int __tmpdw;                           \
	(src).Read(&(tfh).magic,sizeof((tfh).magic));   \
	(src).Read(&__tmpdw,sizeof(unsigned int));      \
	(tfh).version = (int)swabdword(__tmpdw);        \
	(src).Read(&__tmpdw,sizeof(unsigned int));      \
	(tfh).tiles = (int)swabdword(__tmpdw);       \
	(src).Read(&__tmpdw,sizeof(unsigned int));      \
	(tfh).size = (int)swabdword(__tmpdw);       \
	(src).Read(&__tmpdw,sizeof(unsigned int));      \
	(tfh).comp = (int)swabdword(__tmpdw);\
} while (0)

//this is followed by the raw data for the tiles

#endif //ndef __TILEFILE_H
