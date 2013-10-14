#ifndef __SMF_H
#define __SMF_H

#include "byteorder.h"
#include <OpenImageIO/imageio.h>

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
	int tileindexPtr;   // file offset to tile data (see MapTileHeader)
	int minimapPtr;     // file offset to minimap (always 1024*1024 dxt1 compresed data with 9 mipmap sublevels)
	int metalPtr;       // file offset to metalmap (unsigned char[mapx/2 * mapy/2])
	int featurelistPtr; // file offset to feature data (see MapFeatureHeader)
	
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
	(srcptr)->Read((mh).magic,sizeof((mh).magic));  \
	(srcptr)->Read(&__tmpdw,sizeof(unsigned int));  \
	(mh).version = (int)swabdword(__tmpdw);         \
	(srcptr)->Read(&__tmpdw,sizeof(unsigned int));  \
	(mh).id = (int)swabdword(__tmpdw);           \
	(srcptr)->Read(&__tmpdw,sizeof(unsigned int));  \
	(mh).width = (int)swabdword(__tmpdw);            \
	(srcptr)->Read(&__tmpdw,sizeof(unsigned int));  \
	(mh).length = (int)swabdword(__tmpdw);            \
	(srcptr)->Read(&__tmpdw,sizeof(unsigned int));  \
	(mh).squareWidth = (int)swabdword(__tmpdw);      \
	(srcptr)->Read(&__tmpdw,sizeof(unsigned int));  \
	(mh).squareTexels = (int)swabdword(__tmpdw);  \
	(srcptr)->Read(&__tmpdw,sizeof(unsigned int));  \
	(mh).tileTexels = (int)swabdword(__tmpdw);        \
	(srcptr)->Read(&__tmpfloat,sizeof(float));      \
	(mh).floor = swabfloat(__tmpfloat);         \
	(srcptr)->Read(&__tmpfloat,sizeof(float));      \
	(mh).ceiling = swabfloat(__tmpfloat);         \
	(srcptr)->Read(&__tmpdw,sizeof(unsigned int));  \
	(mh).heightPtr = (int)swabdword(__tmpdw);    \
	(srcptr)->Read(&__tmpdw,sizeof(unsigned int));  \
	(mh).typePtr = (int)swabdword(__tmpdw);      \
	(srcptr)->Read(&__tmpdw,sizeof(unsigned int));  \
	(mh).tileindexPtr = (int)swabdword(__tmpdw);        \
	(srcptr)->Read(&__tmpdw,sizeof(unsigned int));  \
	(mh).minimapPtr = (int)swabdword(__tmpdw);      \
	(srcptr)->Read(&__tmpdw,sizeof(unsigned int));  \
	(mh).metalPtr = (int)swabdword(__tmpdw);     \
	(srcptr)->Read(&__tmpdw,sizeof(unsigned int));  \
	(mh).featurelistPtr = (int)swabdword(__tmpdw);      \
	(srcptr)->Read(&__tmpdw,sizeof(unsigned int));  \
	(mh).extraHeaders = (int)swabdword(__tmpdw); \
} while (0)

// start of every extra header must look like this, then comes data specific
// for header type
// struct ExtraHeader {
//     int size; //size of extra header
//     int type;
//     ...
// };



#define SMFEH_NONE 0

#define SMFEH_GRASS 1
// This extension contains a offset to an unsigned char[mapx/4 * mapy/4] array
// that defines ground vegetation.
struct SMFEHGrass {
	SMFEHGrass();
	int size;
	int type;
	int grassPtr;
};

SMFEHGrass::SMFEHGrass()
{
	size = 12;
	type = 1;
	grassPtr = 0;	
}

//some structures used in the chunks of data later in the file

struct SMFHTileindex
{
	int nFiles;
	int nTiles;
};

#define READ_SMFTILEINDEXHEADER(mth,src)             \
do {                                            \
	unsigned int __tmpdw;                       \
	(src).Read(&__tmpdw,sizeof(unsigned int));  \
	(mth).nFiles = swabdword(__tmpdw);    \
	(src).Read(&__tmpdw,sizeof(unsigned int));  \
	(mth).nTiles = swabdword(__tmpdw);        \
} while (0)

#define READPTR_SMFTILEINDEXHEADER(mth,src)          \
do {                                            \
	unsigned int __tmpdw;                       \
	(src)->Read(&__tmpdw,sizeof(unsigned int)); \
	(mth).nFiles = swabdword(__tmpdw);    \
	(src)->Read(&__tmpdw,sizeof(unsigned int)); \
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
	(src)->Read(&__tmpdw,sizeof(unsigned int));     \
	(mfh).nFeatureType = (int)swabdword(__tmpdw); \
	(src)->Read(&__tmpdw,sizeof(unsigned int));     \
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
	(src)->Read(&__tmpdw,sizeof(unsigned int));  \
	(mfs).type = (int)swabdword(__tmpdw); \
	(src)->Read(&__tmpfloat,sizeof(float));      \
	(mfs).x = swabfloat(__tmpfloat);          \
	(src)->Read(&__tmpfloat,sizeof(float));      \
	(mfs).y = swabfloat(__tmpfloat);          \
	(src)->Read(&__tmpfloat,sizeof(float));      \
	(mfs).z = swabfloat(__tmpfloat);          \
	(src)->Read(&__tmpfloat,sizeof(float));      \
	(mfs).r = swabfloat(__tmpfloat);      \
	(src)->Read(&__tmpfloat,sizeof(float));      \
	(mfs).s = swabfloat(__tmpfloat);  \
} while (0)

// Helper Class //
class SMF {
public:
	SMF(bool v, bool q);
	bool verbose, quiet;
	SMFHeader header;
	int width, length;
	float floor, ceiling;

	string in_fn;
	string outPrefix;

	string height_fn;
	string type_fn;

	int tileindexPtr;
	string tileindex_fn;
	vector<string> smt_fn;
	SMFHTileindex tileindexHeader;

	string minimap_fn;
	string metal_fn;

	string featurelist_fn;
	SMFHFeatures featuresHeader;
	vector<string> featureNames;
	vector<SMFFeature> features;
	int featuresPtr;

	string grass_fn;
	SMFEHGrass grassHeader;

	bool load(string filename);

	bool decompileHeight();
	bool decompileType();
	bool decompileMinimap();
	bool decompileMetal();
	bool decompileTileindex();
	bool decompileGrass();
	bool decompileFeaturelist(int format); // 0=csv, 1=lua
	bool decompileAll(int format); // all of the above
//
// reconstructDiffuse(); -> image
//
// Compile Height(image)-> smf
// Compile Type(image)-> smf
// Compile TileIndex(image, vector<string> Filenames)-> smf
// Compile Minimap(image)-> smf
// Compile Metal(image)-> smf
// Compile Grass(image)->smf
// Compile Featurelist(CSV, Lua)-> smf
// Compile ALL() -> smf
//
// Create SMF(filename, width, length, floor ceiling) -> smf
};

SMF::SMF(bool v, bool q)
	: verbose(v), quiet(q)
{
	outPrefix = "out";
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

	in_fn = filename;

	smf.seekg(0);
	smf.read( (char *)&header, sizeof(SMFHeader) );

	// convert internal units to spring map units.
	width = header.width / 64;
	length = header.length / 64;
	floor = header.floor / 512;
	ceiling = header.ceiling / 512;

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

		printf("\tHeightMapPtr: %i\n", header.heightPtr );
		printf("\tTypeMapPtr: %i\n", header.typePtr );
		printf("\tTilesPtr: %i\n", header.tileindexPtr );
		printf("\tMiniMapPtr: %i\n", header.minimapPtr );
		printf("\tMetalMapPtr: %i\n", header.metalPtr );
		printf("\tFeaturePtr: %i\n", header.featurelistPtr );

		printf("\tExtraHeaders: %i\n", header.nExtraHeaders );
	} //fi verbose

	// Extra headers Information
	int memloc;
	int header_size;
	int header_type;
	for(int i = 0; i < header.nExtraHeaders; ++i ) {
		memloc = smf.tellg();
		smf.read( (char *)&header_size, 4);
		smf.read( (char *)&header_type, 4);
		smf.seekg(memloc);
		if(header_type == 1) {
			smf.read( (char *)&grassHeader, sizeof(SMFEHGrass));
			printf("\tGrass: %i\n", grassHeader.grassPtr);
		}
		else if( verbose )printf("WARNING: %i is an unknown header type", i);
		ret = true;
		smf.seekg( memloc + header_size);
	}

	// Tileindex Information
	smf.seekg( header.tileindexPtr );
	smf.read( (char *)&tileindexHeader, sizeof( SMFHTileindex ) );

	if( verbose )printf("INFO: '%i' SMT file(s) referenced\n", tileindexHeader.nFiles);
	
	char temp_fn[256];
	for ( int i=0; i < tileindexHeader.nFiles; ++i) {
		smf.seekg( (int)smf.tellg() + 4 );
		smf.getline( temp_fn, 255, '\0' );
		if( verbose )printf( "\t%s\n", temp_fn );
		smt_fn.push_back(temp_fn);
	}
	tileindexPtr = smf.tellg();

	
	// Featurelist information
	smf.seekg( header.featurelistPtr );
	smf.read( (char *)&featuresHeader, sizeof(SMFHFeatures) );

	if( verbose )printf("INFO: '%i' features, '%i' feature types\n",
		featuresHeader.nFeatures, featuresHeader.nTypes);

	for ( int i=0; i < featuresHeader.nTypes; ++i ) {
		smf.getline( temp_fn, 255, '\0' );
		if( verbose )printf( "\t%s\n", temp_fn );
		featureNames.push_back(temp_fn);
	}
	featuresPtr = smf.tellg();

	smf.close();
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
	fail |= decompileGrass();
	fail |= decompileFeaturelist(format);
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

	if( verbose )printf("INFO: Decompiling Height Map\n");
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

	if( verbose )printf("INFO: Decompiling Type Map\n");
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

	if( verbose )printf("INFO: Decompiling Mini Map\n");
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

	if( verbose )printf("INFO: Decompiling Metal Map\n");
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
	if( verbose )printf("INFO: Decompiling Tile Index\n");
	xres = width * 16;
	yres = length * 16;
	channels = 1;
	size = xres * yres * channels * 4;

	ifstream smf(in_fn.c_str(), ifstream::in);
	if( !smf.good() ) {
		return true;
	}
	pixels = new unsigned int[ size ];
	smf.seekg( tileindexPtr );
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
	for(unsigned int i = 0; i < smt_fn.size(); ++i) {
		list += smt_fn[i] + ',';
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

	if( grassHeader.grassPtr < 80 ) return true;

	if( verbose )printf("INFO: Decompiling Grass Map\n");
	xres = width * 16;
	yres = length * 16;
	channels = 1;
	size = xres * yres * channels;

	ifstream smf(in_fn.c_str(), ifstream::in);
	if( !smf.good() ) {
		return true;
	}

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
SMF::decompileFeaturelist(int format)
{
	if( featuresHeader.nFeatures == 0) return true;
	if( verbose )printf("INFO: Decompiling Feature List\n");

	ifstream smf(in_fn.c_str(), ifstream::in);
	if( !smf.good() ) {
		return true;
	}

	smf.seekg( featuresPtr );
	SMFFeature temp;
	for( int i=0; i < featuresHeader.nFeatures; ++i ) {
		smf.read( (char *)&temp, sizeof(SMFFeature) );
		features.push_back(temp);
	}
	smf.close();

	
	char filename[256];
	if(format == 0)
		sprintf( filename, "%s_featurelist.csv", outPrefix.c_str() );
	else if (format == 1)
		sprintf( filename, "%s_featurelist.lua", outPrefix.c_str() );

	char line[1024];
	ofstream outfile(filename);
	if( outfile.is_open() ) {
		for( unsigned int i = 0; i < features.size(); ++i ) {
			if(format == 0) {
				outfile << featureNames[features[i].type] << ',';
				outfile << features[i].x << ',';
				outfile << features[i].y << ',';
				outfile << features[i].z << ',';
				outfile << features[i].r << ',';
				outfile << features[i].s << endl;
			} else if (format == 1) {
				sprintf(line, "\t\t{ name = '%s', x = %i, z = %i, rot = \"%i\",},\n",
					featureNames[features[i].type].c_str(),
					(int)features[i].x, (int)features[i].z, (int)features[i].r * 32768 );
				outfile << line;
			}

		}
	}
	outfile.close();

	return false;
}
#endif //__SMF_H

