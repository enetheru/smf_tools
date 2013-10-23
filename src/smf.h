#ifndef __SMF_H
#define __SMF_H

#include <OpenImageIO/imageio.h>
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
	char magic[16];     // "spring map file\0"
	int version;        // must be 1 for now
	int id;             // sort of a checksum of the file
	int width;          // map width * 64
	int length;         // map length * 64
	int squareWidth;    // distance between vertices. must be 8
	int squareTexels;   // number of texels per square, must be 8 for now
	int tileTexels;     // number of texels in a tile, must be 32 for now
	float floor;        // height value that 0 in the heightmap corresponds to	
	float ceiling;      // height value that 0xffff in the heightmap corresponds to

	int heightPtr;      // file offset to elevation data (short int[(mapy+1)*(mapx+1)])
	int typePtr;        // file offset to typedata (unsigned char[mapy/2 * mapx/2])
	int tilesPtr;   // file offset to tile data (see MapTileHeader)
	int minimapPtr;     // file offset to minimap (always 1024*1024 dxt1 compresed data with 9 mipmap sublevels)
	int metalPtr;       // file offset to metalmap (unsigned char[mapx/2 * mapy/2])
	int featuresPtr; // file offset to feature data (see MapFeatureHeader)
	
	int nExtraHeaders;   // numbers of extra headers following main header
};

SMFHeader::SMFHeader()
{
	strcpy(magic, "spring map file");
	version = 1;
	squareWidth = 8;
	squareTexels = 8;
	tileTexels = 32;
	return;
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
	SMFEH();
	int size;
	int type;
};

SMFEH::SMFEH()
{
	size = 0;
	type = 0;
}


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
	grassPtr = 0;	
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

// Helper Class //
class SMF {
	bool verbose, quiet;

	// Loading
	SMFHeader header;
	string in_fn;

	// Saving
	bool fast_dxt1;
	vector<SMFEH *> extraHeaders;

	// SMF Information
	int width, length; // values in spring map units
	float floor, ceiling; // values in spring map units

	int heightPtr; 
	
	int typePtr;

	int minimapPtr;

	int metalPtr;

	int grassPtr;

	int tilesPtr;
	int nTiles;			// number of tiles referenced
	vector<string> smtList; // list of smt files references
	vector<int> smtTiles;

	int featuresPtr;
	vector<string> featureTypes;
	vector<SMFFeature> features;

	// Functions
	bool recalculate();
	bool is_smf(string filename);
public:
	string outPrefix;
	string minimapFile;
	string heightFile;
	string grassFile;
	string metalFile;
	string typeFile;
	string tileindexFile;
	string featurelistFile;

	SMF(bool v, bool q);
	bool setDimensions(int width, int length, float floor, float ceiling);

	bool load(string filename);
	bool save();
	bool saveHeight();
	bool saveType();
	bool saveMinimap();
	bool saveMetal();
	bool saveTileindex();
	bool saveFeatures();
	bool saveGrass();

	bool decompileHeight();
	bool decompileType();
	bool decompileMinimap();
	bool decompileMetal();
	bool decompileTileindex();
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
};

SMF::SMF(bool v, bool q)
	: verbose(v), quiet(q)
{
	outPrefix = "out";
	fast_dxt1 = true;
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

	// grassPtr
	grassPtr = metalPtr + width * 32 * length * 32;

	// tilesPtr
	tilesPtr = grassPtr + width * 16 * length * 16;

	// featuresPtr
	featuresPtr = tilesPtr + 8;
	for (unsigned int i = 0; i < smtList.size(); ++i ) {
		featuresPtr += smtList[i].size() + 5;
	}
	featuresPtr += width * 16 * length * 16 * 4;

	return false;
}

bool
SMF::is_smf(string filename)
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

bool
SMF::save()
{
	char filename[256];
	sprintf( filename, "%s.smf", outPrefix.c_str() );
	ofstream smf(filename, ios::binary | ios::out);

	char magic[] = "spring map file\0";
	smf.write( magic, 16 );

	int version = 1;
	smf.write( (char *)&version, 4);

	int id = rand();
	smf.write( (char *)&id, 4);

	int width = this->width * 64;
	smf.write( (char *)&width, 4);

	int length = this->length * 64;
	smf.write( (char *)&length, 4);

	int squareWidth = 8;
	smf.write( (char *)&squareWidth, 4);

	int squareTexels = 8;
	smf.write( (char *)&squareTexels, 4);

	int tileTexels = 32;
	smf.write( (char *)&tileTexels, 4);

	float floor = this->floor * 512;
	smf.write( (char *)&floor, 4);

	float ceiling = this->ceiling * 512;
	smf.write( (char *)&ceiling, 4);

	smf.write( (char *)&heightPtr, 4);
	smf.write( (char *)&typePtr, 4);
	smf.write( (char *)&tilesPtr, 4);
	smf.write( (char *)&minimapPtr, 4);
	smf.write( (char *)&metalPtr, 4);
	smf.write( (char *)&featuresPtr, 4);

	int nExtraHeaders = extraHeaders.size();
	smf.write( (char *)&nExtraHeaders, 4);

	if( strcmp(grassFile.c_str(), "") ){
		int temp = 12;
		smf.write( (char *)&temp, 4);
		temp = 1;
		smf.write( (char *)&temp, 4);
		smf.write( (char *)&grassPtr, 4);
	}
	
	smf.close();

	saveHeight();
	saveType();
	saveMinimap();
	saveMetal();
	if( strcmp( grassFile.c_str(), "") ) saveGrass();
	saveTileindex();
	saveFeatures();

	return false;
}

bool
SMF::saveHeight()
{
	int xres,yres,channels,size;
	ImageInput *imageInput;
	ImageSpec *imageSpec;
	unsigned short *pixels;

	if( verbose ) cout << "INFO: Saving Height. " << heightPtr << endl;

	char filename[256];
	sprintf( filename, "%s.smf", outPrefix.c_str() );

	fstream smf(filename, ios::binary | ios::in| ios::out);
	smf.seekp(heightPtr);
	
	// Dimensions of displacement map.
	xres = width * 64 + 1;
	yres = length * 64 + 1;
	channels = 1;
	size = xres * yres * 2;

	imageInput = NULL;
	if( is_smf(heightFile) ) {
		// Load from SMF
		if( verbose ) printf( "\tSource SMF: %s.\n", heightFile.c_str() );

		imageSpec = new ImageSpec( header.width + 1, header.length + 1,
			channels, TypeDesc::UINT16);
		pixels = new unsigned short [ imageSpec->width * imageSpec->height ];

		ifstream inFile(heightFile.c_str(), ifstream::in);
		inFile.seekg(header.heightPtr);
		inFile.read( (char *)pixels, imageSpec->width * imageSpec->height * 2);
		inFile.close();

	} else if( (imageInput = ImageInput::create( heightFile.c_str() )) ) {
		// load image file
		if( verbose ) printf( "\tSource Image: %s.\n", heightFile.c_str() );

		imageSpec = new ImageSpec;
		imageInput->open( heightFile.c_str(), *imageSpec );
		pixels = new unsigned short [imageSpec->width * imageSpec->height
			* imageSpec->nchannels ];
		imageInput->read_image( TypeDesc::UINT16, pixels );
		imageInput->close();
		delete imageInput;
	} else {
		// Generate blank
		if( verbose ) cout << "\tGenerating flat displacement" << endl;

		imageSpec = new ImageSpec(xres, yres, channels, TypeDesc::UINT16);
		pixels = new unsigned short[ xres * yres ];
		memset( pixels, 0, size);
	}

	// Fix the number of channels
	if( imageSpec->nchannels != channels ) {
		int map[] = {0};
		int fill[] = {0};
		pixels = map_channels(pixels,
			imageSpec->width, imageSpec->height, imageSpec->nchannels,
			channels, map, fill);
	}

	// Fix the size
	if ( imageSpec->width != xres || imageSpec->height != yres ) {
		if( verbose )
			printf( "\tWARNING: %s is (%i,%i), wanted (%i, %i), Resampling.\n",
			heightFile.c_str(), imageSpec->width, imageSpec->height, xres, yres );
		//FIXME Bilinear sampler is broken
//		pixels = interpolate_bilinear( pixels,
		pixels = interpolate_nearest( pixels,
			imageSpec->width, imageSpec->height, channels,
			xres, yres );
	}
	delete imageSpec;

	// FIXME If inversion of height is specified in the arguments,
//	if ( displacement_invert && verbose )
//		cout << "WARNING: Height inversion not implemented yet" << endl;

	// FIXME If inversion of height is specified in the arguments,
//	if ( displacement_lowpass && verbose )
//		cout << "WARNING: Height lowpass filter not implemented yet" << endl;
	
	// write height data to smf.
	smf.write( (char *)pixels, size );
	smf.close();
	delete [] pixels;

	return false;
}

bool
SMF::saveType()
{
	int xres,yres,channels,size;
	ImageInput *imageInput;
	ImageSpec *imageSpec;
	unsigned char *pixels;

	if( verbose ) cout << "INFO: Saving Type. " << typePtr << endl;

	char filename[256];
	sprintf( filename, "%s.smf", outPrefix.c_str() );

	fstream smf(filename, ios::binary | ios::in | ios::out );
	smf.seekp(typePtr);

	// Dimensions of Typemap
	xres = width * 32;
   	yres = length * 32;
	channels = 1;
	size = xres * yres * channels;

	imageInput = NULL;
	if( is_smf(typeFile) ) {
		// Load from SMF
		if( verbose ) printf( "\tSource SMF: %s.\n", typeFile.c_str() );

		imageSpec = new ImageSpec(header.width / 2, header.length / 2,
			channels, TypeDesc::UINT8);
		pixels = new unsigned char[ imageSpec->width * imageSpec->height ];

		ifstream inFile(typeFile.c_str(), ifstream::in);
		inFile.seekg(header.typePtr);
		inFile.read( (char *)pixels, imageSpec->width * imageSpec->height);
		inFile.close();

	} else if((imageInput = ImageInput::create( typeFile.c_str() ))) {
		// Load image file
		if( verbose ) printf("\tSource Image: %s\n", typeFile.c_str() );

		imageSpec = new ImageSpec;
		imageInput->open( typeFile.c_str(), *imageSpec );
		pixels = new unsigned char [imageSpec->width * imageSpec->height
			* imageSpec->nchannels ];
		imageInput->read_image( TypeDesc::UINT8, pixels );
		imageInput->close();
		delete imageInput;
	} else {
		// Generate blank
		if( verbose ) cout << "\tGenerating blank typemap." << endl;
		imageSpec = new ImageSpec(xres, yres, channels, TypeDesc::UINT8);
		pixels = new unsigned char[size];
		memset( pixels, 0, xres * yres);
	}

	// Fix the number of channels
	if( imageSpec->nchannels != channels ) {
		int map[] = {0};
		int fill[] = {0};
		pixels = map_channels(pixels,
			imageSpec->width, imageSpec->height, imageSpec->nchannels,
			channels, map, fill);
	}

	// Fix the Dimensions
	if ( imageSpec->width != xres || imageSpec->height != yres ) {
		if( verbose )
			printf( "\tWARNING: %s is (%i,%i), wanted (%i, %i) Resampling.\n",
			typeFile.c_str(), imageSpec->width, imageSpec->height, xres, yres);

		pixels = interpolate_nearest(pixels,
			imageSpec->width, imageSpec->height, channels,
			xres, yres);	
	}
	delete imageSpec;

	// write the data to the smf
	smf.write( (char *)pixels, size );
	smf.close();
	delete [] pixels;

	return false;
}

bool
SMF::saveMinimap()
{
	// NVTT
	nvtt::InputOptions inputOptions;
	nvtt::OutputOptions outputOptions;
	nvtt::CompressionOptions compressionOptions;
	nvtt::Compressor compressor;
	NVTTOutputHandler *outputHandler;

	//OpenImageIO
	int xres,yres,channels,size;
	ImageInput *imageInput;
	ImageSpec *imageSpec;
	unsigned char *pixels;

	if( verbose ) cout << "INFO: Saving Minimap. " << minimapPtr << endl;

	char filename[256];
	sprintf( filename, "%s.smf", outPrefix.c_str() );

	fstream smf(filename, ios::binary | ios::in | ios::out);
	smf.seekp(minimapPtr);

	// minimap Dimensions
	xres = 1024;
	yres = 1024;
	channels = 4;
	size = xres * yres * channels;

	// Load minimap if possible or try alternatives
	imageInput = NULL;
	if( is_smf(minimapFile) ) { // Copy from SMF
		if( verbose ) printf( "\tSource SMF: %s.\n", minimapFile.c_str() );

		pixels = new unsigned char[MINIMAP_SIZE];

		ifstream inFile(minimapFile.c_str(), ifstream::in);
		inFile.seekg(header.minimapPtr);
		inFile.read( (char *)pixels, MINIMAP_SIZE);
		inFile.close();

		smf.write( (char *)pixels, MINIMAP_SIZE);
		smf.close();
		return false;

	} else if( (imageInput = ImageInput::create( minimapFile.c_str() ))) {
		// Load from Image File
		if( verbose ) printf("\tSource Image %s\n", minimapFile.c_str() );

		imageSpec = new ImageSpec;
		imageInput->open( minimapFile.c_str(), *imageSpec );
		pixels = new unsigned char [imageSpec->width * imageSpec->height
			* imageSpec->nchannels ];
		imageInput->read_image( TypeDesc::UINT8, pixels );
		imageInput->close();
		delete imageInput;
//FIXME attempt to generate minimap from tile files.
//FIXME attempt to create minimap from diffuse image
//FIXME attempt to create minimap from height
	} else {
		// Generate Blank
		if( verbose ) cout << "\tCreating blank minimap." << endl;

		imageSpec = new ImageSpec(xres, yres, channels, TypeDesc::UINT8);
		pixels = new unsigned char[size];
		memset( pixels, 127, size); //grey
	}

	// Fix channels
	int map[] = {2, 1, 0, 3};
	int fill[] = {0, 0, 0, 255};
	pixels = map_channels(pixels,
		imageSpec->width, imageSpec->height, imageSpec->nchannels,
		channels, map, fill);

	// Fix Dimensions
	if ( imageSpec->width != xres || imageSpec->height != yres ) {
		if( verbose )
			printf( "\tWARNING: %s is (%i,%i), wanted (%i, %i), Resampling.\n",
			minimapFile.c_str(), imageSpec->width, imageSpec->height,
			xres, yres);
//FIXME bilinear filter broken
//		pixels = interpolate_bilinear(pixels,
		pixels = interpolate_nearest(pixels,
			imageSpec->width, imageSpec->height, channels,
			xres, yres);
	} 
	delete imageSpec;

	// setup DXT1 Compression
	inputOptions.setTextureLayout( nvtt::TextureType_2D, 1024, 1024 );
	compressionOptions.setFormat( nvtt::Format_DXT1 );

	if( fast_dxt1 ) compressionOptions.setQuality( nvtt::Quality_Fastest ); 
	else compressionOptions.setQuality( nvtt::Quality_Normal ); 
	outputOptions.setOutputHeader( false );

	inputOptions.setMipmapData( pixels, 1024, 1024 );

	outputHandler = new NVTTOutputHandler(MINIMAP_SIZE + 1024);
	outputOptions.setOutputHandler( outputHandler );
	compressor.process( inputOptions, compressionOptions, outputOptions );
	delete [] pixels;

	// Write data to smf
	smf.write( outputHandler->buffer, MINIMAP_SIZE );
	delete outputHandler;

	smf.close();
	return false;
}

bool
SMF::saveMetal()
{
	int xres,yres,channels,size;
	ImageInput *imageInput;
	ImageSpec *imageSpec;
	unsigned char *pixels;

	if( verbose ) cout << "INFO: Saving Metal. " << metalPtr << endl;

	char filename[256];
	sprintf( filename, "%s.smf", outPrefix.c_str() );

	fstream smf(filename, ios::binary | ios::in | ios::out);
	smf.seekp(metalPtr);

	// Dimensions of Metal
	xres = width * 32;
   	yres = length * 32;
	channels = 1;
	size = xres * yres * channels;

	// Open image if possible, or create blank one.
	imageInput = NULL;
	if( is_smf(metalFile) ) {
		if( verbose ) printf( "\tSource SMF %s.\n", metalFile.c_str() );

		imageSpec = new ImageSpec( header.width / 2, header.length / 2,
			channels, TypeDesc::UINT8);
		pixels = new unsigned char [imageSpec->width * imageSpec->height
			* imageSpec->nchannels ];

		ifstream inFile(metalFile.c_str(), ifstream::in);
		inFile.seekg(header.metalPtr);
		inFile.read( (char *)pixels, imageSpec->width * imageSpec->height);
		inFile.close();

	} else if( (imageInput = ImageInput::create( metalFile.c_str() ))) {
		// Load image file
		if( verbose ) printf("\tSource Image: %s\n", metalFile.c_str() );

		imageSpec = new ImageSpec;
		imageInput->open( metalFile.c_str(), *imageSpec );
		pixels = new unsigned char [imageSpec->width * imageSpec->height
			* imageSpec->nchannels ];
		imageInput->read_image( TypeDesc::UINT8, pixels );
		imageInput->close();
		delete imageInput;

	} else {
		if( verbose ) cout << "\tGenerating blank metalmap." << endl;

		imageSpec = new ImageSpec(xres, yres, channels, TypeDesc::UINT8);
		pixels = new unsigned char[size];
		memset( pixels, 0, size);
	}

	// Fix the number of channels
	if( imageSpec->nchannels != channels ) {
		int map[] = {1};
		int fill[] = {0};
		pixels = map_channels(pixels,
			imageSpec->width, imageSpec->height, imageSpec->nchannels,
			channels, map, fill);
	}

	// Fix the size
	if ( imageSpec->width != xres || imageSpec->height != yres ) {
		if( verbose )
			printf( "\tWARNING: %s is (%i,%i), wanted (%i, %i) Resampling.\n",
			metalFile.c_str(), imageSpec->width, imageSpec->height, xres, yres);

		pixels = interpolate_nearest(pixels,
			imageSpec->width, imageSpec->height, channels,
			xres, yres);	
	}
	delete imageSpec;

	// write the data to the smf
	smf.write( (char *)pixels, size );
	smf.close();
	delete [] pixels;

	return false;
}

bool
SMF::saveTileindex()
{
	int xres,yres,channels,size;
	ImageInput *imageInput;
	ImageSpec *imageSpec;
	unsigned int *pixels;

	if( verbose ) cout << "INFO: Saving Tileindex. " << tilesPtr << endl;

	char filename[256];
	sprintf( filename, "%s.smf", outPrefix.c_str() );

	fstream smf(filename, ios::binary | ios::in | ios::out);
	smf.seekp(tilesPtr);

	// Tiles Header
	int nTileFiles = smtList.size();
	smf.write( (char *)&nTileFiles, 4);
	smf.write( (char *)&nTiles, 4);

	// SMT Names
	for(unsigned int i = 0; i < smtList.size(); ++i) {
		if( verbose )printf( "\tAdding %s to list\n", smtList[i].c_str() );
		smf.write( (char *)&smtTiles[i], 4);
		smf.write( smtList[i].c_str(), smtList[i].size() + 1 );
	}

	// Dimensions of Tileindex
	xres = width * 16;
   	yres = length * 16;
	channels = 1;
	size = xres * yres * channels * 4;

	imageInput = NULL;
	if( is_smf(tileindexFile) ) {
		// Load from SMF
		if( verbose ) printf("\tSource SMF %s\n", tileindexFile.c_str() );

		imageSpec = new ImageSpec( header.width / 4, header.length / 4,
			channels, TypeDesc::UINT);
		pixels = new unsigned int [imageSpec->width * imageSpec->height];

		ifstream inFile(tileindexFile.c_str(), ifstream::in);
		inFile.seekg(header.tilesPtr);
		int numFiles = 0;
		inFile.read( (char *)&numFiles, 4);
		inFile.ignore(4);
		for( int i = 0; i < numFiles; ++i ) { 
			inFile.ignore(4);
			inFile.ignore(255, '\0');
		}
		inFile.read( (char *)pixels, imageSpec->width * imageSpec->height * 4);
		inFile.close();

	} else if ( (imageInput = ImageInput::create( tileindexFile.c_str() ))) {
		// Load image file
		if( verbose ) printf("\tSource Image: %s\n", tileindexFile.c_str() );

		imageSpec = new ImageSpec;
		imageInput->open( tileindexFile.c_str(), *imageSpec );
		pixels = new unsigned int [imageSpec->width * imageSpec->height
			* imageSpec->nchannels ];
		imageInput->read_image( TypeDesc::UINT, pixels );
		imageInput->close();
		delete imageInput;
	} else {
		// Generate Consecutive numbering
		if( verbose ) printf( "\tGenerating basic tile index\n" );

		imageSpec = new ImageSpec(xres, yres, channels, TypeDesc::UINT);
		pixels = new unsigned int[ xres * yres * channels ];
		for ( int y = 0; y < yres; ++y )
			for ( int x = 0; x < xres; ++x) {
				pixels[ y * xres + x ] = y * xres + x;
		}
	}

	// Fix the number of channels
	if( imageSpec->nchannels != channels ) {
		int map[] = {1};
		int fill[] = {0};
		pixels = map_channels(pixels,
			imageSpec->width, imageSpec->height, imageSpec->nchannels,
			channels, map, fill);
	}
	// Fix the Dimensions
	if ( imageSpec->width != xres || imageSpec->height != yres ) {
		if( verbose )
			printf( "\tWARNING: %s is (%i,%i), wanted (%i, %i) Resampling.\n",
			metalFile.c_str(), imageSpec->width, imageSpec->height, xres, yres);

		pixels = interpolate_nearest(pixels,
			imageSpec->width, imageSpec->height, channels,
			xres, yres);	
	}
	delete imageSpec;

	// write the data to the smf
	smf.write( (char *)pixels, size );
	delete [] pixels;
	smf.close();

	return false;
}

bool
SMF::saveGrass()
{
	int xres,yres,channels,size;
	ImageInput *imageInput;
	ImageSpec *imageSpec;
	unsigned char *pixels;

	if( verbose ) cout << "INFO: Saving Grass. " << endl;

	// Dimensions of grass
	xres = width * 16;
   	yres = length * 16;
	channels = 1;
	size = xres * yres * channels;

	// Open image if possible, or create blank one.
	imageInput = NULL;
	if( is_smf(grassFile) ) {
		if( verbose ) printf( "\tSource SMF %s.\n", grassFile.c_str() );

		imageSpec = new ImageSpec( header.width / 4, header.length / 4,
			channels, TypeDesc::UINT8);
		pixels = new unsigned char [imageSpec->width * imageSpec->height
			* imageSpec->nchannels ];

		ifstream inFile(grassFile.c_str(), ifstream::in);
		inFile.seekg(80);

		int offset;
		SMFEH extraHeader;
		SMFEHGrass grassHeader;
		for(int i = 0; i < header.nExtraHeaders; ++i ) {
			offset = inFile.tellg();
			inFile.read( (char *)&extraHeader, sizeof(SMFEH) );
			inFile.seekg(offset);
			if(extraHeader.type == 1) {
				inFile.read( (char *)&grassHeader, sizeof(SMFEHGrass));
			}
			inFile.seekg( offset + extraHeader.size);
		}
		inFile.seekg(grassHeader.grassPtr);
		inFile.read( (char *)pixels, imageSpec->width * imageSpec->height);
		inFile.close();

	} else if( (imageInput = ImageInput::create( grassFile.c_str() ))) {
		// Load image file
		if( verbose ) printf("\tSource Image %s\n", grassFile.c_str() );

		imageSpec = new ImageSpec;
		imageInput->open( grassFile.c_str(), *imageSpec );
		pixels = new unsigned char [imageSpec->width * imageSpec->height
			* imageSpec->nchannels ];
		imageInput->read_image( TypeDesc::UINT8, pixels );
		imageInput->close();
		delete imageInput;
	}

	// Fix the number of channels
	if( imageSpec->nchannels != channels ) {
		int map[] = {0};
		int fill[] = {0};
		pixels = map_channels(pixels,
			imageSpec->width, imageSpec->height, imageSpec->nchannels,
			channels, map, fill);
	}

	// Fix the size
	if ( imageSpec->width != xres || imageSpec->height != yres ) {
		if( verbose )
			printf( "\tWARNING: %s is (%i,%i), wanted (%i, %i) Resampling.\n",
			metalFile.c_str(), imageSpec->width, imageSpec->height, xres, yres);

		pixels = interpolate_nearest(pixels,
			imageSpec->width, imageSpec->height, channels,
			xres, yres);	
	}
	delete imageSpec;

	// write the data to the smf
	char filename[256];
	sprintf( filename, "%s.smf", outPrefix.c_str() );

	fstream smf(filename, ios::binary | ios::in | ios::out);
	smf.seekp(grassPtr);

	smf.write( (char *)pixels, size );
	smf.close();
	delete [] pixels;

	return false;
}


bool
SMF::saveFeatures()
{
	if( verbose ) cout << "INFO: Saving Features. " << featuresPtr << endl;

	char filename[256];
	sprintf( filename, "%s.smf", outPrefix.c_str() );

	fstream smf(filename, ios::binary | ios::in | ios::out);
	smf.seekp(featuresPtr);

	int nTypes = featureTypes.size();
	smf.write( (char *)&nTypes, 4); //featuretypes
	int nFeatures = features.size();
	smf.write( (char *)&nFeatures, sizeof(nFeatures)); //numfeatures

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
		= tileindexFile = featurelistFile = filename;
	in_fn = filename;

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
		printf("INFO: Reading %s\n", filename.c_str() );
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
			grassPtr = grassHeader->grassPtr;
			printf("\tGrass: Size %i, type %i, Ptr %i\n",
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

	if( verbose )printf("    SMT file(s) referenced: %i\n", tilesHeader.nFiles);
	
	char temp_fn[256];
	int nTiles;
	for ( int i=0; i < tilesHeader.nFiles; ++i) {
		smf.read( (char *)&nTiles, 4);
		smtTiles.push_back(nTiles);
		smf.getline( temp_fn, 255, '\0' );
		if( verbose )printf( "\t%s\n", temp_fn );
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
	fail |= decompileTileindex();
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
	int xres,yres,channels,size;
	ImageOutput *imageOutput;
	ImageSpec *imageSpec;
	unsigned short *pixels;

	if( verbose )cout << "INFO: Decompiling Height Map. " << header.heightPtr << endl;
	xres = width * 64 + 1;
	yres = length * 64 + 1;
	channels = 1;
	size = xres * yres * channels;
	

	ifstream smf(in_fn.c_str(), ifstream::in);
	if( !smf.good() ) {
		return true;
	}
	pixels = new unsigned short[size];
	smf.seekg( header.heightPtr );
	smf.read( (char *)pixels, size * 2);
	smf.close();

	char filename[256];
	sprintf( filename, "%s_height.tif", outPrefix.c_str() );

	imageOutput = ImageOutput::create(filename);
	if( !imageOutput ) {
		delete [] pixels;
		return true;
	}

	imageSpec = new ImageSpec( xres, yres, channels, TypeDesc::UINT16);
	imageOutput->open( filename, *imageSpec );
	imageOutput->write_image( TypeDesc::UINT16, pixels );
	imageOutput->close();

	delete imageOutput;
	delete imageSpec;
	delete [] pixels;
	return false;
}

bool
SMF::decompileType()
{
	int xres,yres,channels,size;
	ImageOutput *imageOutput;
	ImageSpec *imageSpec;
	unsigned char *pixels;

	if( verbose )cout << "INFO: Decompiling Type Map. " << header.typePtr << endl;
	xres = width * 32;
	yres = length * 32;
	channels = 1;
	size = xres * yres * channels;

	ifstream smf(in_fn.c_str(), ifstream::in);
	if( !smf.good() ) {
		return true;
	}

	pixels = new unsigned char[size];
	smf.seekg(header.typePtr);
	smf.read( (char *)pixels, size);
	smf.close();

	char filename[256];
	sprintf( filename, "%s_type.png", outPrefix.c_str() );

	imageOutput = ImageOutput::create( filename );
	if( !imageOutput ) {
		delete [] pixels;
		return true;
	}

	imageSpec = new ImageSpec( xres, yres, channels, TypeDesc::UINT8);
	imageOutput->open( filename, *imageSpec );
	imageOutput->write_image( TypeDesc::UINT8, pixels );
	imageOutput->close();

	delete imageOutput;
	delete imageSpec;
	delete [] pixels;
	return false;
}

bool
SMF::decompileMinimap()
{
	int xres,yres,channels;
	ImageOutput *imageOutput;
	ImageSpec *imageSpec;
	unsigned char *pixels;

	if( verbose )cout << "INFO: Decompiling Mini Map. " << header.minimapPtr << endl ;
	xres = 1024;
	yres = 1024;
	channels = 4;
	

	ifstream smf(in_fn.c_str(), ifstream::in);
	if( !smf.good() ) {
		return true;
	}

	unsigned char *temp = new unsigned char[MINIMAP_SIZE];
	smf.seekg(header.minimapPtr);
	smf.read( (char *)temp, MINIMAP_SIZE);
	smf.close();

	pixels = dxt1_load(temp, xres, yres);
	delete [] temp;


	char filename[256];
	sprintf( filename, "%s_minimap.png", outPrefix.c_str() );

	imageOutput = ImageOutput::create(filename);
	if( !imageOutput ) {
		delete [] pixels;
		return true;
	}

	imageSpec = new ImageSpec( xres, yres, channels, TypeDesc::UINT8);
	imageOutput->open( filename, *imageSpec );
	imageOutput->write_image( TypeDesc::UINT8, pixels );
	imageOutput->close();

	delete imageOutput;
	delete imageSpec;
	delete [] pixels;

	return false;
}

bool
SMF::decompileMetal()
{
	int xres,yres,channels,size;
	ImageOutput *imageOutput;
	ImageSpec *imageSpec;
	unsigned char *pixels;

	if( verbose )cout << "INFO: Decompiling Metal Map. " << header.metalPtr << endl;
	xres = width * 32;
	yres = length * 32;
	channels = 1;
	size = xres * yres * channels;
	

	ifstream smf(in_fn.c_str(), ifstream::in);
	if( !smf.good() ) {
		return true;
	}

	pixels = new unsigned char[size];
	smf.seekg( header.metalPtr );
	smf.read( (char *)pixels, size );
	smf.close();

	char filename[256];
	sprintf( filename, "%s_metal.png", outPrefix.c_str() );

	imageOutput = ImageOutput::create(filename);
	if( !imageOutput ) {
		delete [] pixels;
		return true;
	}

	imageSpec = new ImageSpec( xres, yres, channels, TypeDesc::UINT8);
	imageOutput->open( filename, *imageSpec );
	imageOutput->write_image( TypeDesc::UINT8, pixels );
	imageOutput->close();

	delete imageOutput;
	delete imageSpec;
	delete [] pixels;

	return false;
}

bool
SMF::decompileTileindex()
{
	int xres,yres,channels,size;
	ImageOutput *imageOutput;
	ImageSpec *imageSpec;
	unsigned int *pixels;

	// Output TileIndex
	if( verbose )cout << "INFO: Decompiling Tile Index. " << header.tilesPtr << endl;
	xres = width * 16;
	yres = length * 16;
	channels = 1;
	size = xres * yres * channels * 4;

	ifstream smf(in_fn.c_str(), ifstream::in);
	if( !smf.good() ) {
		return true;
	}
	smf.seekg( header.tilesPtr );
	int numFiles = 0;
	smf.read( (char *)&numFiles, 4);
	smf.ignore(4);
	for( int i = 0; i < numFiles; ++i ) { 
		smf.ignore(4);
		smf.ignore(255, '\0');
	}

	pixels = new unsigned int[ size ];
	smf.read( (char *)pixels, size );
	smf.close();

	char filename[256];
	sprintf( filename, "%s_tileindex.exr", outPrefix.c_str() );

	imageOutput = ImageOutput::create(filename);
	if( !imageOutput ) {
		delete [] pixels;
		return true;
	}

	imageSpec = new ImageSpec( xres, yres, channels, TypeDesc::UINT);
	string list;
	for(unsigned int i = 0; i < smtList.size(); ++i) {
		list += smtList[i] + ',';
	}
	imageSpec->attribute("smt_list", list);
	imageOutput->open( filename, *imageSpec );
	imageOutput->write_image( TypeDesc::UINT, pixels );
	imageOutput->close();

	delete imageOutput;
	delete imageSpec;
	delete [] pixels;
	return false;
}

bool
SMF::decompileGrass()
{
	int xres,yres,channels,size;
	ImageOutput *imageOutput;
	ImageSpec *imageSpec;
	unsigned char *pixels;
	bool grass = false;

	xres = width * 16;
	yres = length * 16;
	channels = 1;
	size = xres * yres * channels;

	ifstream smf(in_fn.c_str(), ifstream::in);
	if( !smf.good() ) {
		return true;
	}

	int offset;
	SMFEH extraHeader;
	SMFEHGrass grassHeader;
	for(int i = 0; i < header.nExtraHeaders; ++i ) {
		offset = smf.tellg();
		smf.read( (char *)&extraHeader, sizeof(SMFEH) );
		smf.seekg(offset);
		if(extraHeader.type == 1) {
			grass = true;
			smf.read( (char *)&grassHeader, sizeof(SMFEHGrass));
		}
		smf.seekg( offset + extraHeader.size);
	}
	if(!grass)return true;

	if( verbose )cout << "INFO: Decompiling Grass Map. " << grassHeader.grassPtr << endl;

	pixels = new unsigned char[size];
	smf.seekg( grassHeader.grassPtr );
	smf.read( (char *)pixels, size );
	smf.close();

	char filename[256];
	sprintf( filename, "%s_grass.png", outPrefix.c_str() );

	imageOutput = ImageOutput::create(filename);
	if( !imageOutput ) {
		delete [] pixels;
		return true;
	}

	imageSpec = new ImageSpec( xres, yres, channels, TypeDesc::UINT8);
	imageOutput->open( filename, *imageSpec );
	imageOutput->write_image( TypeDesc::UINT8, pixels );
	imageOutput->close();

	delete imageOutput;
	delete imageSpec;
	delete [] pixels;
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

	ifstream smf(in_fn.c_str(), ifstream::in);
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
#endif //__SMF_H

