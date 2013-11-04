#ifndef __SMF_H
#define __SMF_H

#include <cstring>
#include <string>
#include <vector>
using namespace std;

#include <OpenImageIO/imagebuf.h>
OIIO_NAMESPACE_USING

// Minimap size is defined by a DXT1 compressed 1024x1024 image with 8 mipmaps.
// 1024   + 512    + 256   + 128  + 64   + 32  + 16  + 8  + 4
// 524288 + 131072 + 32768 + 8192 + 2048 + 512 + 128 + 32 + 8 = 699048
#define MINIMAP_SIZE 699048

/*
map file (.smf) layout is like this

MapHeader

ExtraHeader
ExtraHeader
...
Chunk of data pointed to by header or extra headers
Chunk of data pointed to by header or extra headers
...
*/

struct SMFHeader {
	SMFHeader();
/*  0 */	char magic[16];     // "spring map file\0"
/* 16 */	int version;        // must be 1 for now
/* 20 */	int id;             // sort of a checksum of the file
/* 24 */	int width;          // map width * 64
/* 28 */	int length;         // map length * 64
/* 32 */	int squareWidth;    // distance between vertices. must be 8
/* 36 */	int squareTexels;   // number of texels per square, must be 8 for now
/* 40 */	int tileTexels;     // number of texels in a tile, must be 32 for now
/* 44 */	float floor;        // height value that 0 in the heightmap corresponds to	
/* 48 */	float ceiling;      // height value that 0xffff in the heightmap corresponds to

/* 52 */	int heightPtr;      // file offset to elevation data (short int[(mapy+1)*(mapx+1)])
/* 56 */	int typePtr;        // file offset to typedata (unsigned char[mapy/2 * mapx/2])
/* 60 */	int tilesPtr;   // file offset to tile data (see MapTileHeader)
/* 64 */	int minimapPtr;     // file offset to minimap (always 1024*1024 dxt1 compresed data with 9 mipmap sublevels)
/* 68 */	int metalPtr;       // file offset to metalmap (unsigned char[mapx/2 * mapy/2])
/* 72 */	int featuresPtr; // file offset to feature data (see MapFeatureHeader)
	
/* 76 */	int nExtraHeaders;   // numbers of extra headers following main header
};/* 80 */

#define READPTR_SMTHEADER(mh,srcptr)                \
do {                                                \
	unsigned int __tmpdw;                           \
	float __tmpfloat;                               \
	(srcptr)->read((mh).magic,sizeof((mh).magic));  \
	(srcptr)->read(&__tmpdw,sizeof(unsigned int));  \
	(mh).version = (int)swabdword(__tmpdw);         \
	(srcptr)->read(&__tmpdw,sizeof(unsigned int));  \
	(mh).id = (int)swabdword(__tmpdw);           \
	(srcptr)->read(&__tmpdw,sizeof(unsigned int));  \
	(mh).width = (int)swabdword(__tmpdw);            \
	(srcptr)->read(&__tmpdw,sizeof(unsigned int));  \
	(mh).length = (int)swabdword(__tmpdw);            \
	(srcptr)->read(&__tmpdw,sizeof(unsigned int));  \
	(mh).squareWidth = (int)swabdword(__tmpdw);      \
	(srcptr)->read(&__tmpdw,sizeof(unsigned int));  \
	(mh).squareTexels = (int)swabdword(__tmpdw);  \
	(srcptr)->read(&__tmpdw,sizeof(unsigned int));  \
	(mh).tileTexels = (int)swabdword(__tmpdw);        \
	(srcptr)->read(&__tmpfloat,sizeof(float));      \
	(mh).floor = swabfloat(__tmpfloat);         \
	(srcptr)->read(&__tmpfloat,sizeof(float));      \
	(mh).ceiling = swabfloat(__tmpfloat);         \
	(srcptr)->read(&__tmpdw,sizeof(unsigned int));  \
	(mh).heightPtr = (int)swabdword(__tmpdw);    \
	(srcptr)->read(&__tmpdw,sizeof(unsigned int));  \
	(mh).typePtr = (int)swabdword(__tmpdw);      \
	(srcptr)->read(&__tmpdw,sizeof(unsigned int));  \
	(mh).tileindexPtr = (int)swabdword(__tmpdw);        \
	(srcptr)->read(&__tmpdw,sizeof(unsigned int));  \
	(mh).minimapPtr = (int)swabdword(__tmpdw);      \
	(srcptr)->read(&__tmpdw,sizeof(unsigned int));  \
	(mh).metalPtr = (int)swabdword(__tmpdw);     \
	(srcptr)->read(&__tmpdw,sizeof(unsigned int));  \
	(mh).featurelistPtr = (int)swabdword(__tmpdw);      \
	(srcptr)->read(&__tmpdw,sizeof(unsigned int));  \
	(mh).extraHeaders = (int)swabdword(__tmpdw); \
} while (0)

// start of every extra header must look like this, then comes data specific
// for header type
struct SMFEH {
	int size;
	int type;
};

#define SMFEH_NONE 0

#define SMFEH_GRASS 1
// This extension contains a offset to an unsigned char[mapx/4 * mapy/4] array
// that defines ground vegetation.
struct SMFEHGrass: public SMFEH
{
	SMFEHGrass();
	int grassPtr;
};


//some structures used in the chunks of data later in the file

struct SMFHTiles
{
	int nFiles;
	int nTiles;
};

#define READ_SMFTILEINDEXHEADER(mth,src)             \
do {                                            \
	unsigned int __tmpdw;                       \
	(src).read( (char *)&__tmpdw,sizeof(unsigned int));  \
	(mth).nFiles = swabdword(__tmpdw);    \
	(src).read( (char *)&__tmpdw,sizeof(unsigned int));  \
	(mth).nTiles = swabdword(__tmpdw);        \
} while (0)

#define READPTR_SMFTILEINDEXHEADER(mth,src)          \
do {                                            \
	unsigned int __tmpdw;                       \
	(src)->read( (char *)&__tmpdw,sizeof(unsigned int)); \
	(mth).nFiles = swabdword(__tmpdw);    \
	(src)->read( (char *)&__tmpdw,sizeof(unsigned int)); \
	(mth).nTiles = swabdword(__tmpdw);        \
} while (0)

/* this is followed by numTileFiles file definition where each file definition
   is an int followed by a zero terminated file name. Each file defines as
   many tiles the int indicates with the following files starting where the
   last one ended. So if there is 2 files with 100 tiles each the first
   defines 0-99 and the second 100-199. After this followes an
   int[ mapx * texelPerSquare / tileSize * mapy * texelPerSquare / tileSize ]
   which is indexes to the defined tiles */

struct SMFHFeatures 
{
	int nTypes;    // number of feature types
	int nFeatures; // number of features
};

#define READ_SMFFEATURESHeader(mfh,src)              \
do {                                                \
	unsigned int __tmpdw;                           \
	(src)->read( (char *)&__tmpdw,sizeof(unsigned int));     \
	(mfh).nFeatureType = (int)swabdword(__tmpdw); \
	(src)->read( (char *)&__tmpdw,sizeof(unsigned int));     \
	(mfh).numFeatures = (int)swabdword(__tmpdw);    \
} while (0)

/* this is followed by numFeatureType zero terminated strings indicating the
   names of the features in the map then follow numFeatures
   MapFeatureStructs */

struct SMFFeature
{
	int type;        // index to one of the strings above
	float x,y,z,r,s; // position, rotation, scale
};

#define READ_SMFFEATURE(mfs,src)           \
do {                                             \
	unsigned int __tmpdw;                        \
	float __tmpfloat;                            \
	(src)->read( (char *)&__tmpdw,sizeof(unsigned int));  \
	(mfs).type = (int)swabdword(__tmpdw); \
	(src)->read( (char *)&__tmpfloat,sizeof(float));      \
	(mfs).x = swabfloat(__tmpfloat);          \
	(src)->read( (char *)&__tmpfloat,sizeof(float));      \
	(mfs).y = swabfloat(__tmpfloat);          \
	(src)->read( (char *)&__tmpfloat,sizeof(float));      \
	(mfs).z = swabfloat(__tmpfloat);          \
	(src)->read( (char *)&__tmpfloat,sizeof(float));      \
	(mfs).r = swabfloat(__tmpfloat);      \
	(src)->read( (char *)&__tmpfloat,sizeof(float));      \
	(mfs).s = swabfloat(__tmpfloat);  \
} while (0)

bool is_smf(string filename);

// Helper Class //
class SMF {

	// Loading
	SMFHeader header;
	string loadFile;

	// Saving
	string outPrefix;
	vector<SMFEH *> extraHeaders;

	// SMF Information
	int width, length; // values in spring map units
	float floor, ceiling; // values in spring map units

	int heightPtr; 
	string heightFile;
	
	int typePtr;
	string typeFile;

	int minimapPtr;
	string minimapFile;

	int metalPtr;
	string metalFile;

	string grassFile;

	int tilesPtr;
	int nTiles;			// number of tiles referenced
	vector<string> smtList; // list of smt files references
	vector<int> smtTiles; //number of tiles in files from smtList
	string tilemapFile;

	int featuresPtr;
	vector<string> featureTypes;
	vector<SMFFeature> features;
	string featuresFile;

	// Functions
	bool recalculate();

public:
	bool verbose, quiet, slowcomp, invert;

	SMF();
	SMF( string loadFile );
	void setOutPrefix(string prefix);
	bool setDimensions(int width, int length, float floor, float ceiling);
	void setHeightFile( string filename );
	void setTypeFile( string filename );
	void setMinimapFile( string filename );
	void setMetalFile( string filename );
	void setTilemapFile( string filename );
	void setFeaturesFile( string filename );
	void setGrassFile(string filename);

	void unsetGrassFile();

	void addTileFile( string filename );

	bool load(string filename);

	bool save();
	bool save(string filename);
	bool saveHeight();
	bool saveType();
	bool saveMinimap();
	bool saveMetal();
	bool saveTilemap();
	bool saveFeatures();
	bool saveGrass();

	bool decompileHeight();
	bool decompileType();
	bool decompileMinimap();
	bool decompileMetal();
	bool decompileTilemap();
	bool decompileGrass();
	bool decompileFeaturelist(int format); // 0=csv, 1=lua
	bool decompileAll(int format); // all of the above

	ImageBuf * getHeight();
	ImageBuf * getType();
	ImageBuf * getMinimap();
	ImageBuf * getMetal();
	ImageBuf * getTilemap();
	ImageBuf * getGrass();

};
#endif //__SMF_H
