#ifndef __SMF_H
#define __SMF_H

#include <stdio.h>

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>

#include <nvtt/nvtt.h>

#include "byteorder.h"
#include "nvtt_output_handler.h"

// Minimap size is defined by a DXT1 compressed 1024x1024 image with 8 mipmaps.
// 1024   + 512    + 256   + 128  + 64   + 32  + 16  + 8  + 4
// 524288 + 131072 + 32768 + 8192 + 2048 + 512 + 128 + 32 + 8 = 699048
#define MINIMAP_SIZE 699048

OIIO_NAMESPACE_USING
using namespace std;

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

SMFHeader::SMFHeader()
:	version(1),
	squareWidth(8),
	squareTexels(8),
	tileTexels(32)
{
	strcpy(magic, "spring map file");
}

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

SMFEHGrass::SMFEHGrass()
{
	size = 12;
	type = 1;
}

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
	bool invert;
	
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
	bool verbose, quiet, slowcomp;

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
//TODO
// reconstructDiffuse(); -> image
//
// Compile Grass(image)->smf
// Compile Featurelist(CSV, Lua)-> smf
// Compile ALL() -> smf
//
// Create SMF(filename, width, length, floor ceiling) -> smf
//	bool create(string filename, int width, int length, float floor, float ceiling, bool grass);

	ImageBuf * getHeight();
	ImageBuf * getType();
	ImageBuf * getMinimap();
	ImageBuf * getMetal();
	ImageBuf * getTilemap();
	ImageBuf * getGrass();

};

SMF::SMF()
:	verbose(true),
	quiet(false),
	slowcomp(false)
{
	outPrefix = "out";
	nTiles = 0;
	invert = false;
}

SMF::SMF(string loadFile)
:	verbose(true),
	quiet(false),
	slowcomp(false)
{
	outPrefix = "out";
	nTiles = 0;
	invert = false;
	load(loadFile);
}

// This function makes sure that all file offsets are valid. and should be
// called whenever changes to the class are made that will effect its values.
bool
SMF::recalculate()
{
	// heightPtr
	int offset = 0;
	for (unsigned int i = 0; i < extraHeaders.size(); ++i ) {
		offset += extraHeaders[i]->size;
	}
	heightPtr = sizeof(SMFHeader) + offset;

	// TypePtr
	typePtr = heightPtr + ((width * 64 + 1) * (length * 64 + 1) * 2);

	// minimapPtr
	minimapPtr = typePtr + width * 32 * length * 32;

	// metalPtr
	metalPtr = minimapPtr + MINIMAP_SIZE;

	// get optional grass header
	SMFEHGrass *grassHeader = NULL;
	for(unsigned int i = 0; i < extraHeaders.size(); ++i ) {
		if( extraHeaders[i]->type == 1) {
			grassHeader = reinterpret_cast<SMFEHGrass *>(extraHeaders[i]);
		}
	}

	// set the tilesPtr and grassPtr
	if(grassHeader) {
		grassHeader->grassPtr = metalPtr + width * 32 * length * 32;
		tilesPtr = grassHeader->grassPtr + width * 16 * length * 16;
	} else {
		tilesPtr = metalPtr + width * 32 * length * 32;
	}

	// featuresPtr
	featuresPtr = tilesPtr + 8;
	for (unsigned int i = 0; i < smtList.size(); ++i ) {
		featuresPtr += smtList[i].size() + 5;
	}
	featuresPtr += width * 16 * length * 16 * 4;

	return false;
}

bool
is_smf(string filename)
{
	char magic[16];
	// Perform tests for file validity
	ifstream smf(filename.c_str(), ifstream::in);
	if( smf.good() ) {
		smf.read( magic, 16);
		// perform test for file format
		if( !strcmp(magic, "spring map file") ) {
			smf.close();
			return true;
		}
	}
	return false;
}

void
SMF::setOutPrefix(string prefix)
{
	outPrefix = prefix.c_str();
}

bool
SMF::setDimensions(int width, int length, float floor, float ceiling)
{
	this->width = width;
	this->length = length;
	this->floor = floor;
	this->ceiling = ceiling;
	recalculate();
	return false;
}

void SMF::setHeightFile(   string filename ) { heightFile    = filename.c_str(); }
void SMF::setTypeFile(     string filename ) { typeFile      = filename.c_str(); }
void SMF::setMinimapFile(  string filename ) { minimapFile   = filename.c_str(); }
void SMF::setMetalFile(    string filename ) { metalFile     = filename.c_str(); }
void SMF::setTilemapFile(  string filename ) { tilemapFile   = filename.c_str(); }
void SMF::setFeaturesFile( string filename ) { featuresFile  = filename.c_str(); }

void
SMF::addTileFile( string filename )
{
	char magic[16];
	int nTiles;
	//FIXME load up SMT and get number of tiles stored in the file. maybe.
	fstream smt(filename.c_str(), ios::in | ios::binary);
	smt.read(magic, 16);
	if( strcmp(magic, "spring tilefile")) {
		if( !quiet )printf( "ERROR: %s is not a valid smt file, ignoring.\n", filename.c_str());
		return;
	}
	smt.ignore(4);
	smt.read( (char *)&nTiles, 4);
	this->nTiles += nTiles;
	smtTiles.push_back(nTiles);
	smtList.push_back(filename);

	recalculate();
}

void
SMF::setGrassFile(string filename)
{
	SMFEHGrass *grassHeader = NULL;
	for(unsigned int i = 0; i < extraHeaders.size(); ++i ) {
		if(extraHeaders[i]->type == 1) {
			grassHeader = reinterpret_cast<SMFEHGrass *>(extraHeaders[i]);
		}
	}
	if(grassHeader == NULL) {
		grassHeader = new SMFEHGrass;
		extraHeaders.push_back(reinterpret_cast<SMFEH *>(grassHeader));
	}
	grassFile = filename.c_str();
	recalculate();
}

void
SMF::unsetGrassFile()
{
	for(unsigned int i = 0; i < extraHeaders.size(); ++i ) {
		if(extraHeaders[i]->type == 1) {
			extraHeaders.erase(extraHeaders.begin() + i);
		}
	}
	recalculate();
}

bool
SMF::save(string filename)
{
	outPrefix = filename.c_str();
	cout << outPrefix << endl;
	return save();
}

bool
SMF::save()
{
	char filename[256];
	sprintf( filename, "%s.smf", outPrefix.c_str() );
	ofstream smf(filename, ios::binary | ios::out);
	if( verbose )printf( "\nINFO: Saving %s.\n", filename );

	char magic[] = "spring map file\0";
	smf.write( magic, 16 );

	int version = 1;
	smf.write( (char *)&version, 4);
	if( verbose )printf( "\tVersion: %i.\n", version );

	int id = rand();
	smf.write( (char *)&id, 4);
	if( verbose )printf( "\tMapID: %i.\n", id );

	int width = this->width * 64;
	smf.write( (char *)&width, 4);
	if( verbose )printf( "\tWidth: %i.\n", width );

	int length = this->length * 64;
	smf.write( (char *)&length, 4);
	if( verbose )printf( "\tLength: %i.\n", length );

	int squareWidth = 8;
	smf.write( (char *)&squareWidth, 4);
	if( verbose )printf( "\tSquareWidth: %i.\n", squareWidth );

	int squareTexels = 8;
	smf.write( (char *)&squareTexels, 4);
	if( verbose )printf( "\tSquareTexels: %i.\n", squareTexels );

	int tileTexels = 32;
	smf.write( (char *)&tileTexels, 4);
	if( verbose )printf( "\tTileTexels: %i.\n", tileTexels );

	float floor = this->floor * 512;
	smf.write( (char *)&floor, 4);
	if( verbose )printf( "\tMinHeight: %0.2f.\n", floor );

	float ceiling = this->ceiling * 512;
	smf.write( (char *)&ceiling, 4);
	if( verbose )printf( "\tMaxHeight: %0.2f.\n", ceiling );

	smf.write( (char *)&heightPtr, 4);
	if( verbose )printf( "\tHeightPtr: %i.\n", heightPtr );
	smf.write( (char *)&typePtr, 4);
	if( verbose )printf( "\tTypePtr: %i.\n", typePtr );
	smf.write( (char *)&tilesPtr, 4);
	if( verbose )printf( "\tTilesPtr: %i.\n", tilesPtr );
	smf.write( (char *)&minimapPtr, 4);
	if( verbose )printf( "\tMinimapPtr: %i.\n", minimapPtr );
	smf.write( (char *)&metalPtr, 4);
	if( verbose )printf( "\tMetalPtr: %i.\n", metalPtr );
	smf.write( (char *)&featuresPtr, 4);
	if( verbose )printf( "\tFeaturesPtr: %i.\n", featuresPtr );

	int nExtraHeaders = extraHeaders.size();
	smf.write( (char *)&nExtraHeaders, 4);
	if( verbose )printf( "\tExtraHeaders: %i.\n", nExtraHeaders );

	if(verbose) printf( "    Extra Headers:\n");
	for( unsigned int i = 0; i < extraHeaders.size(); ++i ) {
		smf.write((char *)extraHeaders[i], extraHeaders[i]->size);
		if( verbose ) {
			if(extraHeaders[i]->type == 1)
				printf("\tGrassPtr: %i.\n",
				((SMFEHGrass *)extraHeaders[i])->grassPtr );
			else printf( "\t Unknown Header\n");
		}
	}
	
	smf.close();

	saveHeight();
	saveType();
	saveMinimap();
	saveMetal();

	for( unsigned int i = 0; i < extraHeaders.size(); ++i ) {
		if(extraHeaders[i]->type == 1)saveGrass();
	}

	saveTilemap();
	saveFeatures();

	return false;
}

bool
SMF::saveHeight()
{
	if( verbose )cout << "INFO: saveHeight\n";
	// Dimensions of displacement map.
	ImageBuf *imageBuf = NULL;
	ROI roi(	0, width * 64 + 1,  // xbegin, xend
				0, length * 64 + 1, // ybegin, yend
				0, 1,               // zbegin, zend
				0, 1);              // chbegin, chend
	ImageSpec imageSpec( roi.xend, roi.yend, roi.chend, TypeDesc::UINT16 );

	if( is_smf(heightFile) ) {
		// Load from SMF
		SMF sourcesmf(heightFile);
		imageBuf = sourcesmf.getHeight();
	}
   	if( !imageBuf ) {
		// load image file
		imageBuf = new ImageBuf( heightFile );
		imageBuf->read( 0, 0, false, TypeDesc::UINT16 );
		if( !imageBuf->initialized() ) {
			delete imageBuf;
			imageBuf = NULL;
		}
	}
	if( !imageBuf ) {
		// Generate blank
		imageBuf = new ImageBuf( "height", imageSpec );
	}

	imageSpec = imageBuf->specmod();
	ImageBuf fixBuf;
	// Fix the number of channels
	if( imageSpec.nchannels != roi.chend ) {
		int map[] = {0};
		ImageBufAlgo::channels(fixBuf, *imageBuf, roi.chend, map);
		imageBuf->copy(fixBuf);
		fixBuf.clear();
	}

	// Fix the size
	if ( imageSpec.width != roi.xend || imageSpec.height != roi.yend ) {
		if( verbose )
			printf( "\tWARNING: %s is (%i,%i), wanted (%i, %i), Resampling.\n",
			heightFile.c_str(), imageSpec.width, imageSpec.height, roi.xend, roi.yend );
		ImageBufAlgo::resample(fixBuf, *imageBuf, true, roi);
		imageBuf->copy(fixBuf);
		fixBuf.clear();
	}

	// Invert height
	if ( invert ) {
		ImageSpec fixSpec(roi.xend, roi.yend, roi.chend, TypeDesc::UINT16);
		fixBuf.reset( "fixBuf",  fixSpec );
		const float fill[] = {65535};
		ImageBufAlgo::fill(fixBuf, fill);
		ImageBufAlgo::sub(*imageBuf, fixBuf, *imageBuf);
		fixBuf.clear();
	}

	// FIXME filter to remove stepping artifacts from 8bit images,
//	if ( lowpass ) {
//	}
	
	unsigned short *pixels = (unsigned short *)imageBuf->localpixels();

	// write height data to smf.
	char filename[256];
	sprintf( filename, "%s.smf", outPrefix.c_str() );

	fstream smf(filename, ios::binary | ios::in| ios::out);
	smf.seekp(heightPtr);

	smf.write( (char *)pixels, imageBuf->spec().image_bytes() );
	smf.close();
	delete imageBuf;
	if( is_smf( heightFile ) ) delete [] pixels;

	return false;
}

bool
SMF::saveType()
{
	if( verbose )cout << "INFO: saveType\n";

	ImageBuf *imageBuf = NULL;
	ROI roi(	0, width * 32,
				0, length * 32,
				0, 1,
				0, 1);
	ImageSpec imageSpec(roi.xend, roi.yend, roi.chend, TypeDesc::UINT8 );

	if( is_smf(typeFile) ) {
		// Load from SMF
		SMF sourcesmf(typeFile);
		imageBuf = sourcesmf.getType();
	}
	if( !imageBuf ) {
		// Load image file
		imageBuf = new ImageBuf( typeFile );
		imageBuf->read( 0, 0, false, TypeDesc::UINT8 );
		if( !imageBuf->initialized() ) {
			delete imageBuf;
			imageBuf = NULL;
		}
	}
	if( !imageBuf ) {
		// Generate blank
		imageBuf = new ImageBuf( "type", imageSpec );
	}

	imageSpec = imageBuf->specmod();
	ImageBuf fixBuf;

	// Fix the number of channels
	if( imageSpec.nchannels != roi.chend ) {
		int map[] = {0};
		ImageBufAlgo::channels(fixBuf, *imageBuf, roi.chend, map );
		imageBuf->copy( fixBuf );
		fixBuf.clear();
	}

	// Fix the Dimensions
	if ( imageSpec.width != roi.xend || imageSpec.height != roi.yend ) {
		if( verbose )
			printf( "\tWARNING: %s is (%i,%i), wanted (%i, %i) Resampling.\n",
			typeFile.c_str(), imageSpec.width, imageSpec.height, roi.xend, roi.yend);
		ImageBufAlgo::resample(fixBuf, *imageBuf, false, roi);
		imageBuf->copy( fixBuf );
		fixBuf.clear();		
	}

	unsigned char *pixels = (unsigned char *)imageBuf->localpixels();

	// write the data to the smf
	char filename[256];
	sprintf( filename, "%s.smf", outPrefix.c_str() );

	fstream smf(filename, ios::binary | ios::in | ios::out );
	smf.seekp(typePtr);

	smf.write( (char *)pixels, imageBuf->spec().image_bytes() );
	smf.close();
	delete imageBuf;
	if( is_smf(typeFile) )delete [] pixels;

	return false;
}

bool
SMF::saveMinimap()
{
	if( verbose )cout << "INFO: saveMinimap\n";

	char filename[256];
	sprintf( filename, "%s.smf", outPrefix.c_str() );

	fstream smf(filename, ios::binary | ios::in | ios::out);
	smf.seekp(minimapPtr);

	unsigned char *pixels;
	if( is_smf(minimapFile) ) {
		// Copy from SMF
		pixels = new unsigned char[MINIMAP_SIZE];

		ifstream inFile(minimapFile.c_str(), ifstream::in);
		inFile.seekg(header.minimapPtr);
		inFile.read( (char *)pixels, MINIMAP_SIZE);
		inFile.close();

		smf.write( (char *)pixels, MINIMAP_SIZE);
		smf.close();
		delete [] pixels;
		return false;
	}

	//OpenImageIO
	ROI roi(	0, 1024,
				0, 1024,
				0, 1,
				0, 4);
	ImageSpec imageSpec( roi.xend, roi.yend, roi.chend, TypeDesc::UINT8 );
	

	// Load image file
	ImageBuf *imageBuf = new ImageBuf( minimapFile );
	imageBuf->read( 0, 0, false, TypeDesc::UINT8 );
//FIXME attempt to generate minimap from tile files.

	if( !imageBuf->initialized() ) {
		// Create from height
		imageBuf->reset( minimapFile );
		imageBuf->read( 0, 0, false, TypeDesc::UINT8 );
	}

	if( !imageBuf->initialized() ) {
		// Create blank
		imageBuf->reset( "minimap", imageSpec);
	}

	imageSpec = imageBuf->specmod();
	ImageBuf fixBuf;
	// Fix channels
	if( imageSpec.nchannels != roi.chend ) {
		int map[] = {2,1,0,3};
		float fill[] = {0,0,0,255};
		ImageBufAlgo::channels(fixBuf, *imageBuf, roi.chend, map, fill);
		imageBuf->copy(fixBuf);
		fixBuf.clear();
	}

	// Fix dimensions
	if( imageSpec.width != roi.xend || imageSpec.height != roi.yend ) {
		printf( "\tWARNING: %s is (%i,%i), wanted (%i, %i), Resampling.\n",
			minimapFile.c_str(), imageSpec.width, imageSpec.height, roi.xend, roi.yend );
		ImageBufAlgo::resample(fixBuf, *imageBuf, true, roi);
		imageBuf->copy(fixBuf);
		fixBuf.clear();
	}

	pixels = (unsigned char *)imageBuf->localpixels();

	// setup DXT1 Compression
	nvtt::InputOptions inputOptions;
	inputOptions.setTextureLayout( nvtt::TextureType_2D, 1024, 1024 );
	inputOptions.setMipmapData( pixels, 1024, 1024 );

	nvtt::CompressionOptions compressionOptions;
	compressionOptions.setFormat( nvtt::Format_DXT1 );
	if( slowcomp ) compressionOptions.setQuality( nvtt::Quality_Normal ); 
	else compressionOptions.setQuality( nvtt::Quality_Fastest ); 

	nvtt::OutputOptions outputOptions;
	outputOptions.setOutputHeader( false );

	NVTTOutputHandler *outputHandler = new NVTTOutputHandler(MINIMAP_SIZE + 1);
	outputOptions.setOutputHandler( outputHandler );

	nvtt::Compressor compressor;
	compressor.process( inputOptions, compressionOptions, outputOptions );

	// Write data to smf
	smf.write( outputHandler->buffer, MINIMAP_SIZE );
	delete outputHandler;

	smf.close();
	delete imageBuf;		
	return false;
}

bool
SMF::saveMetal()
{
	if( verbose )cout << "INFO: saveMetal\n";

	// Dimensions of displacement map.
	ImageBuf *imageBuf = NULL;
	ROI roi(	0, width * 32,  // xbegin, xend
				0, length * 32, // ybegin, yend
				0, 1,               // zbegin, zend
				0, 1);              // chbegin, chend
	ImageSpec imageSpec( roi.xend, roi.yend, roi.chend, TypeDesc::UINT8 );

	if( is_smf(metalFile) ) {
		// Load from smf
		SMF sourcesmf(metalFile);
		imageBuf = sourcesmf.getMetal();
	}
	if( !imageBuf ) {
		//load from image
		imageBuf = new ImageBuf(metalFile);
		imageBuf->read( 0, 0, false, TypeDesc::UINT8 );
		if( !imageBuf->initialized() ) {
			delete imageBuf;
			imageBuf = NULL;
		}
	}
	if( !imageBuf ) {
		// Generate blank
		imageBuf = new ImageBuf( "metal", imageSpec );
	}

	imageSpec = imageBuf->specmod();
	ImageBuf fixBuf;
	// Fix the number of channels
	if( imageSpec.nchannels != roi.chend ) {
		int map[] = {0};
		ImageBufAlgo::channels(fixBuf, *imageBuf, roi.chend, map);
		imageBuf->copy(fixBuf);
		fixBuf.clear();
	}

	// Fix the size
	if ( imageSpec.width != roi.xend || imageSpec.height != roi.yend ) {
		if( verbose )
			printf( "\tWARNING: %s is (%i,%i), wanted (%i, %i), Resampling.\n",
			metalFile.c_str(), imageSpec.width, imageSpec.height, roi.xend, roi.yend );
		ImageBufAlgo::resample(fixBuf, *imageBuf, true, roi);
		imageBuf->copy(fixBuf);
		fixBuf.clear();
	}

	unsigned char *pixels = (unsigned char *)imageBuf->localpixels();


	char filename[256];
	sprintf( filename, "%s.smf", outPrefix.c_str() );

	fstream smf(filename, ios::binary | ios::in | ios::out);
	smf.seekp(metalPtr);

	// write the data to the smf
	smf.write( (char *)pixels, imageBuf->spec().image_bytes() );
	smf.close();

	delete imageBuf;
	if( is_smf( metalFile ) ) delete [] pixels;

	return false;
}

bool
SMF::saveTilemap()
{
	if( verbose )cout << "INFO: saveTilemap\n";

	char filename[256];
	sprintf( filename, "%s.smf", outPrefix.c_str() );

	fstream smf(filename, ios::binary | ios::in | ios::out);
	smf.seekp(tilesPtr);

	// Tiles Header
	int nTileFiles = smtList.size();
	smf.write( (char *)&nTileFiles, 4);
	smf.write( (char *)&nTiles, 4);
	if(verbose)printf( "    %i tiles referenced in %i files\n", nTiles, nTileFiles );

	// SMT Names
	for(unsigned int i = 0; i < smtList.size(); ++i) {
		if( verbose )printf( "\t%i %s\n", smtTiles[i], smtList[i].c_str() );
		smf.write( (char *)&smtTiles[i], 4);
		smf.write( smtList[i].c_str(), smtList[i].size() +1 );
	}

	// Dimensions of displacement map.
	ImageBuf *imageBuf = NULL;
	ROI roi(	0, width * 16,  // xbegin, xend
				0, length * 16, // ybegin, yend
				0, 1,               // zbegin, zend
				0, 1);              // chbegin, chend
	ImageSpec imageSpec( roi.xend, roi.yend, roi.chend, TypeDesc::UINT );

	if( is_smf(tilemapFile) ) {
		// Load from SMF
		SMF sourcesmf(tilemapFile);
		imageBuf = sourcesmf.getTilemap();
	}
   	if( !imageBuf ) {
		// load image file
		imageBuf = new ImageBuf( tilemapFile );
		imageBuf->read( 0, 0, false, TypeDesc::UINT );
		if( !imageBuf->initialized() ) {
			delete imageBuf;
			imageBuf = NULL;
		}
	}
	if( !imageBuf ) {
		// Generate blank
		imageBuf = new ImageBuf( "tilemap", imageSpec );
		for ( unsigned int i = 0; i < imageSpec.image_pixels(); ++i )
			((unsigned int *)imageBuf->localpixels())[ i ] = i;
	}

	imageSpec = imageBuf->specmod();
	ImageBuf fixBuf;
	// Fix the number of channels
	if( imageSpec.nchannels != roi.chend ) {
		int map[] = {0};
		ImageBufAlgo::channels(fixBuf, *imageBuf, roi.chend, map);
		imageBuf->copy(fixBuf);
		fixBuf.clear();
	}

	// Fix the size
	// FIXME image should never be resized, instead tiling either from an edge or centred.
	if ( imageSpec.width != roi.xend || imageSpec.height != roi.yend ) {
		if( verbose )
			printf( "\tWARNING: %s is (%i,%i), wanted (%i, %i), Resampling.\n",
			tilemapFile.c_str(), imageSpec.width, imageSpec.height, roi.xend, roi.yend );
		ImageBufAlgo::resample(fixBuf, *imageBuf, false, roi);
		imageBuf->copy(fixBuf);
		fixBuf.clear();
	}

	unsigned int *pixels = (unsigned int *)imageBuf->localpixels();

	// write the data to the smf
	smf.write( (char *)pixels, imageBuf->spec().image_bytes() );
	smf.close();

	delete imageBuf;
	if( is_smf( tilemapFile ) ) delete [] pixels;

	return false;
}

bool
SMF::saveGrass()
{
	if( verbose )cout << "INFO: saveGrass\n";

	SMFEHGrass *grassHeader = NULL;
	for( unsigned int i = 0; i < extraHeaders.size(); ++i ) {
		if( extraHeaders[ i ]->type == 1 )
			grassHeader = reinterpret_cast<SMFEHGrass *>( extraHeaders[ i ] );
	}
	if( !grassHeader )return true;

	ImageBuf *imageBuf = NULL;
	ROI roi(	0, width * 16,
				0, length * 16,
				0, 1,
				0, 1);
	ImageSpec imageSpec(roi.xend, roi.yend, roi.chend, TypeDesc::UINT8 );

	if( is_smf(grassFile) ) {
		// Load from SMF
		SMF sourcesmf(grassFile);
		imageBuf = sourcesmf.getGrass();
	}
	if( !imageBuf ) {
		// Load image file
		imageBuf = new ImageBuf( grassFile );
		imageBuf->read( 0, 0, false, TypeDesc::UINT8 );
		if( imageBuf->initialized() ) {
			delete imageBuf;
			imageBuf = NULL;
		}
	}
	if( !imageBuf ) {
		// Generate blank
		imageBuf = new ImageBuf( "grass", imageSpec );
	}

	imageSpec = imageBuf->specmod();
	ImageBuf fixBuf;

	// Fix the number of channels
	if( imageSpec.nchannels != roi.chend ) {
		int map[] = {0};
		ImageBufAlgo::channels(fixBuf, *imageBuf, roi.chend, map );
		imageBuf->copy( fixBuf );
		fixBuf.clear();
	}

	// Fix the Dimensions
	if ( imageSpec.width != roi.xend || imageSpec.height != roi.yend ) {
		if( verbose )
			printf( "\tWARNING: %s is (%i,%i), wanted (%i, %i) Resampling.\n",
			typeFile.c_str(), imageSpec.width, imageSpec.height, roi.xend, roi.yend);
		ImageBufAlgo::resample(fixBuf, *imageBuf, false, roi);
		imageBuf->copy( fixBuf );
		fixBuf.clear();		
	}

	unsigned char *pixels = (unsigned char *)imageBuf->localpixels();

	char filename[256];
	sprintf( filename, "%s.smf", outPrefix.c_str() );

	if( verbose )printf( "    Source: %s.\n", grassFile.c_str() );

	fstream smf(filename, ios::binary | ios::in | ios::out);
	smf.seekp(grassHeader->grassPtr);

	smf.write( (char *)pixels, imageBuf->spec().image_bytes() );
	smf.close();
	
	delete imageBuf;
	if( is_smf( grassFile ) ) delete [] pixels;

	return false;
}


bool
SMF::saveFeatures()
{
	if( verbose )cout << "INFO: saveFeatures\n";

	char filename[256];
	sprintf( filename, "%s.smf", outPrefix.c_str() );

	fstream smf(filename, ios::binary | ios::in | ios::out);
	smf.seekp(featuresPtr);

	int nTypes = featureTypes.size();
	smf.write( (char *)&nTypes, 4); //featuretypes
	int nFeatures = features.size();
	smf.write( (char *)&nFeatures, sizeof(nFeatures)); //numfeatures
	if( verbose ) printf( "    %i features, %i types.\n", nFeatures, nTypes);

	for(unsigned int i = 0; i < featureTypes.size(); ++i ) {
		smf.write(featureTypes[i].c_str(), featureTypes[i].size() + 1 );
	}

	for(unsigned int i = 0; i < features.size(); ++i ) {
		smf.write( (char *)&features[i], sizeof(SMFFeature) );
	}

	smf.write( "\0", sizeof("\0"));
	smf.close();
	return false;
}

bool
SMF::load(string filename)
{
	bool ret = false;
	char magic[16];

	// Perform tests for file validity
	ifstream smf(filename.c_str(), ifstream::in);
	if( !smf.good() ) {
		if ( !quiet )printf( "ERROR: Cannot open %s\n", filename.c_str() ); 
		ret = true;
	}
	
	smf.read( magic, 16);
	// perform test for file format
	if( strcmp(magic, "spring map file") ) {
		if( !quiet )printf("ERROR: %s is not a valid smf file.\n", filename.c_str() );
		ret = true;
	}

	if( ret ) return ret;

	heightFile = typeFile = minimapFile = metalFile = grassFile
		= tilemapFile = featuresFile = filename;
	loadFile = filename;

	smf.seekg(0);
	smf.read( (char *)&header, sizeof(SMFHeader) );

	// convert internal units to spring map units.
	width = header.width / 64;
	length = header.length / 64;
	floor = (float)header.floor / 512;
	ceiling = (float)header.ceiling / 512;
	heightPtr = header.heightPtr;
	typePtr = header.typePtr;
	tilesPtr = header.tilesPtr;
	minimapPtr = header.minimapPtr;
	metalPtr = header.metalPtr;
	featuresPtr = header.featuresPtr;

	if( verbose ) {
		printf("INFO: Loading %s\n", filename.c_str() );
		printf("\tVersion: %i\n", header.version );
		printf("\tID: %i\n", header.id );

		printf("\tWidth: %i\n", width );
		printf("\tLength: %i\n", length );
		printf("\tSquareSize: %i\n", header.squareWidth );
		printf("\tTexelPerSquare: %i\n", header.squareTexels );
		printf("\tTileSize: %i\n", header.tileTexels );
		printf("\tMinHeight: %f\n", floor );
		printf("\tMaxHeight: %f\n", ceiling );

		printf("\tHeightMapPtr: %i\n", heightPtr );
		printf("\tTypeMapPtr: %i\n", typePtr );
		printf("\tTilesPtr: %i\n", tilesPtr );
		printf("\tMiniMapPtr: %i\n", minimapPtr );
		printf("\tMetalMapPtr: %i\n", metalPtr );
		printf("\tFeaturePtr: %i\n", featuresPtr );

		printf("\tExtraHeaders: %i\n", header.nExtraHeaders );
	} //fi verbose

	// Extra headers Information
	int offset;
	SMFEH extraHeader;
	for(int i = 0; i < header.nExtraHeaders; ++i ) {
		if( verbose )cout << "    Extra Headers" << endl;
		offset = smf.tellg();
		smf.read( (char *)&extraHeader, sizeof(SMFEH) );
		smf.seekg(offset);
		if(extraHeader.type == 0) {
			if( verbose )printf("\tNUll type: 0\n");
			continue;
		}
		if(extraHeader.type == 1) {
			SMFEHGrass *grassHeader = new SMFEHGrass;
			smf.read( (char *)grassHeader, sizeof(SMFEHGrass));
			if( verbose )printf("\tGrass: Size %i, type %i, Ptr %i\n",
				grassHeader->size, grassHeader->type, grassHeader->grassPtr);
			extraHeaders.push_back( (SMFEH *)grassHeader );
		} else {
			ret = true;
			if( verbose )printf("WARNING: %i is an unknown header type\n", i);
		}
		smf.seekg( offset + extraHeader.size);
	}

	// Tileindex Information
	SMFHTiles tilesHeader;
	smf.seekg( header.tilesPtr );
	smf.read( (char *)&tilesHeader, sizeof( SMFHTiles ) );
	nTiles = tilesHeader.nTiles;

	if( verbose )printf("    %i tiles referenced in %i files.\n", tilesHeader.nTiles, tilesHeader.nFiles);
	
	char temp_fn[256];
	int nTiles;
	for ( int i=0; i < tilesHeader.nFiles; ++i) {
		smf.read( (char *)&nTiles, 4);
		smtTiles.push_back(nTiles);
		smf.getline( temp_fn, 255, '\0' );
		if( verbose )printf( "\t%i %s\n", nTiles, temp_fn );
		smtList.push_back(temp_fn);
	}

	// Featurelist information
	smf.seekg( header.featuresPtr );
	int nTypes;
	smf.read( (char *)&nTypes, 4);
	int nFeatures;
	smf.read( (char *)&nFeatures, 4);

	if( verbose )printf("    Features: %i, Feature types: %i\n",
			nFeatures, nTypes);
	
	offset = smf.tellg();
	smf.seekg (0, ios::end);
	int filesize = smf.tellg();
	smf.seekg(offset);

	int eeof = offset + nFeatures * sizeof(SMFFeature);
	if( eeof > filesize ) {
		if( !quiet )printf( "WARNING: Filesize is not large enough to contain the reported number of features. Ignoring feature data.\n");
		nFeatures = 0;
		nTypes = 0;
	}


	char temp[256];
	for ( int i = 0; i < nTypes; ++i ) {
		smf.getline(temp, 255, '\0' );
		featureTypes.push_back(temp);
	}

	SMFFeature feature;
	for ( int i = 0; i < nFeatures; ++i ) {
		smf.read( (char *)&feature, sizeof(SMFFeature) );
		features.push_back(feature);
	}


	smf.close();
	recalculate();
	return false;
}

bool
SMF::decompileAll(int format)
{
	bool fail = false;
	fail |= decompileHeight();
	fail |= decompileType();
	fail |= decompileMinimap();
	fail |= decompileMetal();
	fail |= decompileTilemap();
	fail |= decompileFeaturelist(format);
	fail |= decompileGrass();
	if(fail) {
		if(verbose)cout << "WARNING: something failed to export correctly" << endl;
	}
	return fail;
}
bool
SMF::decompileHeight()
{
	ImageBuf *imageBuf = getHeight();
	if( !imageBuf ) return true;

	char filename[256];
	sprintf( filename, "%s_height.tif", outPrefix.c_str() );

	imageBuf->save( filename );
	return false;
}

bool
SMF::decompileType()
{
	ImageBuf *imageBuf = getType();
	if( !imageBuf ) return true;

	char filename[256];
	sprintf( filename, "%s_type.png", outPrefix.c_str() );

	imageBuf->save( filename );
	return false;
}

bool
SMF::decompileMinimap()
{
	ImageBuf *imageBuf = getMinimap();
	if( !imageBuf ) return true;

	char filename[256];
	sprintf( filename, "%s_minimap.png", outPrefix.c_str() );

	imageBuf->save( filename );
	return false;
}

bool
SMF::decompileMetal()
{
	ImageBuf *imageBuf = getMetal();
	if( !imageBuf ) return true;

	char filename[256];
	sprintf( filename, "%s_metal.png", outPrefix.c_str() );

	imageBuf->save( filename );
	return false;
}

bool
SMF::decompileTilemap()
{
	ImageBuf *imageBuf = getTilemap();
	if( !imageBuf ) return true;

	char filename[256];
	sprintf( filename, "%s_tilemap.exr", outPrefix.c_str() );

	imageBuf->save( filename );
	return false;
}

bool
SMF::decompileGrass()
{
	ImageBuf *imageBuf = getGrass();
	if( !imageBuf ) return true;

	char filename[256];
	sprintf( filename, "%s_grass.png", outPrefix.c_str() );

	imageBuf->save( filename );
	return false;
}

bool
SMF::decompileFeaturelist(int format = 0)
{
	if( features.size() == 0) return true;
	if( verbose )cout << "INFO: Decompiling Feature List. "
		<< header.featuresPtr << endl;

	char filename[256];
	if(format == 0)
		sprintf( filename, "%s_featurelist.csv", outPrefix.c_str() );
	else if (format == 1)
		sprintf( filename, "%s_featurelist.lua", outPrefix.c_str() );

	ofstream outfile(filename);

	ifstream smf(loadFile.c_str(), ifstream::in);
	if( !smf.good() ) {
		return true;
	}

	smf.seekg( header.featuresPtr );

	int nTypes;
	smf.read( (char *)&nTypes, 4);

	int nFeatures;
	smf.read( (char *)&nFeatures, 4);

	char name[256];
	vector<string> featureNames;
	for( int i = 0; i < nTypes; ++i ) {
		smf.getline(name,255, '\0');
		featureNames.push_back(name);
	}

	char line[1024];
	SMFFeature feature;
	for( int i=0; i < nFeatures; ++i ) {
//		READ_SMFFEATURE(temp,&smf);
		smf.read( (char *)&feature, sizeof(SMFFeature) );

		if(format == 0) {
				outfile << featureNames[feature.type] << ',';
				outfile << feature.x << ',';
				outfile << feature.y << ',';
				outfile << feature.z << ',';
				outfile << feature.r << ',';
				outfile << feature.s << endl;
			} else if (format == 1) {
				sprintf(line, "\t\t{ name = '%s', x = %i, z = %i, rot = \"%i\",},\n",
					featureNames[feature.type].c_str(),
					(int)feature.x, (int)feature.z, (int)feature.r * 32768 );
				outfile << line;
			}

	}
	smf.close();
	outfile.close();

	return false;
}

ImageBuf *
SMF::getHeight()
{
	ImageBuf *imageBuf = NULL;
	ImageSpec imageSpec( header.width + 1, header.length + 1, 1, TypeDesc::UINT16 );
	unsigned short *data = new unsigned short[ imageSpec.image_pixels() ];

	ifstream smf( loadFile.c_str() );
	if( smf.good() ) {
		smf.seekg(header.heightPtr);
		smf.read( (char *)data, imageSpec.image_bytes() );
		imageBuf = new ImageBuf( "height", imageSpec, data);
	}
	smf.close();
	return imageBuf;
}

ImageBuf *
SMF::getType()
{
	ImageBuf * imageBuf = NULL;
	ImageSpec imageSpec( header.width / 2, header.length / 2, 1, TypeDesc::UINT8 );
	unsigned char *data = new unsigned char[ imageSpec.image_pixels() ];

	ifstream smf( loadFile.c_str() );
	if( smf.good() ) {
		smf.seekg( header.typePtr );
		smf.read( (char *)data, imageSpec.image_bytes() );
		imageBuf = new ImageBuf( "type", imageSpec, data );
	}
	smf.close();
	return imageBuf;
}

ImageBuf *
SMF::getMinimap()
{
	ImageBuf * imageBuf = NULL;
	ImageSpec imageSpec( 1024, 1024, 4, TypeDesc::UINT8 );
	unsigned char *data;

	ifstream smf( loadFile.c_str() );
	if( smf.good() ) {
		unsigned char *temp = new unsigned char[MINIMAP_SIZE];
		smf.seekg( header.minimapPtr );
		smf.read( (char *)temp, MINIMAP_SIZE);
		data = dxt1_load(temp, 1024, 1024);
		delete [] temp;
		imageBuf = new ImageBuf( "minimap", imageSpec, data );
	}
	smf.close();
	return imageBuf;
}

ImageBuf *
SMF::getMetal()
{
	ImageBuf * imageBuf = NULL;
	ImageSpec imageSpec( header.width / 2, header.length / 2, 1, TypeDesc::UINT8 );
	unsigned char *data = new unsigned char[ imageSpec.image_pixels() ];

	ifstream smf( loadFile.c_str() );
	if( smf.good() ) {
		smf.seekg( header.metalPtr );
		smf.read( (char *)data, imageSpec.image_bytes() );
		imageBuf = new ImageBuf( "metal", imageSpec, data );
	}
	smf.close();
	return imageBuf;
}

ImageBuf *
SMF::getTilemap()
{
	ImageBuf * imageBuf = NULL;
	ImageSpec imageSpec( header.width / 4, header.length / 4, 1, TypeDesc::UINT );
	unsigned int *data = new unsigned int[ imageSpec.image_pixels() ];

	ifstream smf( loadFile.c_str() );
	if( smf.good() ) {
		smf.seekg( header.tilesPtr );
		int numFiles = 0;
		smf.read( (char *)&numFiles, 4 );
		smf.ignore( 4 );
		for( int i = 0; i < numFiles; ++i ) { 
			smf.ignore(4);
			smf.ignore(255, '\0');
		}

		smf.read( (char *)data, imageSpec.image_bytes() );
		imageBuf = new ImageBuf( "tilemap", imageSpec, data );
	}
	smf.close();
	return imageBuf;
}

ImageBuf *
SMF::getGrass()
{
	ImageBuf * imageBuf = NULL;
	ImageSpec imageSpec( header.width / 4, header.length / 4, 1, TypeDesc::UINT8 );
	unsigned char *data = new unsigned char[ imageSpec.image_pixels() ];

	ifstream smf( loadFile.c_str() );
	if( smf.good() ) {
		smf.seekg(80);

		int offset;
		SMFEH extraHeader;
		SMFEHGrass grassHeader;
		for(int i = 0; i < header.nExtraHeaders; ++i ) {
			offset = smf.tellg();
			smf.read( (char *)&extraHeader, sizeof(SMFEH) );
			smf.seekg(offset);
			if(extraHeader.type == 1) {
				smf.read( (char *)&grassHeader, sizeof(SMFEHGrass));
			}
			smf.seekg( offset + extraHeader.size);
		}
		smf.seekg(grassHeader.grassPtr);
		smf.read( (char *)data, imageSpec.image_bytes() );
		imageBuf = new ImageBuf( "grass", imageSpec, data );
	}
	smf.close();
	return imageBuf;
}
#endif //__SMF_H

//FIXME incorporate
/*	// Process Features //
	//////////////////////
	if( featurelist ) {
		if( verbose ) cout << "INFO: Processing and writing features." << endl;

		MapFeatureHeader mapFeatureHeader;
		vector<MapFeatureStruct> features;
		MapFeatureStruct *feature;
		vector<string> featurenames;

		string line;
		string cell;
		vector<string> tokens;
		char value[256];

		// build inbuilt list
		for ( int i=0; i < 16; i++ ) {
			sprintf(value, "TreeType%i", i);
			featurenames.push_back(value);
		}
		featurenames.push_back("GeoVent");

		// Parse the file
		printf( "INFO: Reading %s\n", featurelist_fn.c_str() );
		ifstream flist(featurelist_fn.c_str(), ifstream::in);

		while ( getline( flist, line ) ) {
		    stringstream lineStream( line );
			feature = new MapFeatureStruct;
			mapFeatureHeader.numFeatures++;

			tokens.clear();
		    while( getline( lineStream, cell, ',' ) ) tokens.push_back( cell );

			if(!featurenames.empty()) {
				for (unsigned int i=0; i < featurenames.size(); i++)
					if( !featurenames[i].compare( tokens[0] ) ) {
						feature->featureType = i;
						break;
					} else {
						featurenames.push_back(tokens[0]);
						feature->featureType = i;
						break;
					}
			}
			feature->xpos = atof(tokens[1].c_str());
			feature->ypos = atof(tokens[2].c_str());
			feature->zpos = atof(tokens[3].c_str());
			feature->rotation = atof(tokens[4].c_str());
			feature->relativeSize = atof(tokens[5].c_str());

			features.push_back(*feature);
		}
		mapFeatureHeader.numFeatureType = featurenames.size();
		printf("INFO: %i Features, %i Types\n", mapFeatureHeader.numFeatures,
				   	mapFeatureHeader.numFeatureType );

		header.featurePtr = smf_of.tellp();
		smf_of.seekp(76);
		smf_of.write( (char *)&header.featurePtr, 4);
		smf_of.seekp(header.featurePtr);
		smf_of.write( (char *)&mapFeatureHeader, sizeof( mapFeatureHeader ) );

		vector<string>::iterator i;
		for ( i = featurenames.begin(); i != featurenames.end(); ++i ) {
			string c = *i;
			smf_of.write( c.c_str(), (int)strlen( c.c_str() ) + 1 );
		}

		vector<MapFeatureStruct>::iterator fi;
		for( fi = features.begin(); fi != features.end(); ++fi ) {
			smf_of.write( (char *)&*fi, sizeof( MapFeatureStruct ) );
		}
	}

	smf_of.close();*/

