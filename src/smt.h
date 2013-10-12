#ifndef __SMT_H
#define __SMT_H

#include "byteorder.h"
#include <OpenImageIO/imageio.h>

OIIO_NAMESPACE_USING

#define COMP_DXT1 1

// struct used when comparing tiles.
struct TileMip
{
	TileMip();
	char crc[8];
	char n;
};

TileMip::TileMip()
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
	SMTHeader();
	char magic[16];   //"spring tilefile\0"
	int version;      //must be 1 for now
	int count;        //total number of tiles in this file
	int res;     //must be 32 for now
	int type;     //must be 1=dxt1 for now
};

SMTHeader::SMTHeader()
{
	strcpy(magic,"spring tilefile");
	version = 1;
	res = 32;
	type = 1;
}


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
	SMTHeader header;

	// Tiles
	vector<TileMip> hash;

	// Sources
	vector<string> sourceFileNames;
	int sourceImageStride;

	// Tile Index
	unsigned int *tileIndex;
	int tileIndex_w, tileIndex_l;
	int tileSize;

	// Map Information
	int width,length;

	// Output
	string output_fn;
	string input_fn;

	bool quiet, verbose;

	SMT();
	SMT(string sourceImage, int w, int l);
	~SMT();
	bool setSourceImages(vector<string> fileNames, int stride);
	bool setSourceImages(string fileName);
	void setType(int type);

	bool load(string fileName);
	bool loadTileIndex(string fileName);
	bool save();
	bool decompileTiles(string prefix);
	bool decompileCollate(string prefix);
	bool decompileReconstruct(string prefix);
};

SMT::SMT()
	: sourceImageStride(0), tileIndex(NULL), tileSize(0), width(0), length(0),
		output_fn(""), input_fn(""), quiet(false), verbose(false)
{
	header.count = 0;
	return;
}
SMT::SMT(string sourceImage, int w, int l)
	: sourceImageStride(1), tileIndex(NULL), tileSize(0), width(w), length(l),
		output_fn(""), input_fn(""), quiet(false), verbose(false)
{
	sourceFileNames.push_back(sourceImage);
}

SMT::~SMT()
{
	//TODO make sure you are deleting all data.
	if (tileIndex) delete [] tileIndex;
	return;
}

bool
SMT::load(string fileName)
{
	input_fn = fileName;
	ifstream inFile;
	char magic[16];
	bool fail = false;
	// Test File
	if( fileName.compare("") ) {
		inFile.open(fileName.c_str(), ifstream::in);
		if(!inFile.good()) {
			if ( !quiet )printf( "ERROR: Cannot open '%s'\n", fileName.c_str() ); 
			fail = true;
		} else {
			inFile.read(magic, 16);
			if( strcmp(magic, "spring tilefile") ) {
				if ( !quiet )printf( "ERROR: %s is not a valid smt\n", fileName.c_str() );
				fail = true;
			}
		}
	} //fileName.compare("")
	if ( fail ) {
		inFile.close();
		return fail;

	}

	inFile.seekg(0);
	inFile.read( (char *)&header, sizeof(SMTHeader) );

	if( verbose ) {
		cout << "INFO: " << fileName << endl;
		cout << "\tSMT Version: " << header.version << endl;
		cout << "\tTiles: " << header.count << endl;
		cout << "\tTile Size: " << header.res << endl;
		cout << "\tCompression: ";
		if(header.type == 1)cout << "dxt1" << endl;
		else {
			cout << "UNKNOWN" << endl;
			fail = true;
		}
	}
	setType(header.type);

	inFile.close();
	return fail;
}

bool
SMT::loadTileIndex(string fileName)
{
	// OpenImageIO
	ImageInput *imageInput = NULL;
	ImageSpec *imageSpec = NULL;

	//TODO, load from SMF, CSV
	imageInput = NULL;
	imageInput = ImageInput::create( fileName.c_str() );
	if( !imageInput ) {
		if( !quiet )cout << "ERROR: reading tile index" << endl;
		return true;
	} else {
		if( verbose ) printf( "INFO: Reading %s.\n", fileName.c_str() );
		imageSpec = new ImageSpec;
		imageInput->open( fileName.c_str(), *imageSpec );
		tileIndex_w = imageSpec->width;
		tileIndex_l = imageSpec->height;
		tileIndex = new unsigned int[ imageSpec->width * imageSpec->height * imageSpec->nchannels ];
		imageInput->read_image( TypeDesc::UINT, tileIndex );
		imageInput->close();
		delete imageInput;
		delete imageSpec;
	}
	return true;
}

bool
SMT::save()
{
	//TODO
	return true;
}

bool
SMT::decompileTiles(string prefix)
{
	bool fail = false;
	ImageOutput *imageOutput;
	ImageSpec *imageSpec;
	unsigned char *tile_data;
	unsigned char *pixels;

	if( !input_fn.compare("")) {
		if( !quiet )cout << "ERROR: no smt file loaded" << endl;
		return true;
	}

	ifstream inFile(input_fn.c_str(), ifstream::in);
	inFile.seekg( sizeof(SMTHeader) );

	// Save each tile to an individual file
	for(int i = 0; i < header.count && !fail; ++i ) {
		// Pull data
		tile_data = new unsigned char[tileSize];
		inFile.read( (char *)tile_data, tileSize);
		pixels = dxt1_load(tile_data, header.res, header.res);
		delete [] tile_data;

		char filename[256];
		sprintf(filename, "%s_tile_%010i.jpg", prefix.c_str(), i);

		imageOutput = NULL;
		imageOutput = ImageOutput::create(filename);
		if(!imageOutput) fail = true;
		else {
			imageSpec = new ImageSpec(header.res, header.res, 4, TypeDesc::UINT8);
			imageOutput->open( filename, *imageSpec );
			imageOutput->write_image( TypeDesc::UINT8, pixels);
			imageOutput->close();
			delete imageOutput;
			delete imageSpec;
		}
		delete [] pixels;
	}
	return fail;
}

bool
SMT::decompileCollate(string prefix)
{
	// OpenImageIO
	ImageOutput *imageOutput = NULL;
	ImageSpec *imageSpec = NULL;
	int xres, yres, channels;
	int tileindex_w, tileindex_l;
	unsigned char *pixels;
	unsigned char *tpixels;
	unsigned char *tileData;
	ifstream infile;

	// Allocate Image size large enough to accomodate the tiles,
	tileindex_w = tileindex_l = ceil(sqrt(header.count));
	xres = yres = tileindex_w * header.res;
	channels = 4;
	pixels = new unsigned char[xres * yres * channels];

	char filename[256];
	sprintf(filename, "%s_tiles.jpg", prefix.c_str());

	imageOutput = NULL;
	imageOutput = ImageOutput::create(filename);
	if(!imageOutput) return true;
	imageSpec = new ImageSpec(xres, yres, channels, TypeDesc::UINT8);

	// Loop through tiles copying the data to our image buffer
	//TODO save scanlines to file once a row is complete to
	//save on memory
	infile.open(input_fn.c_str(), ifstream::in);
	infile.seekg( sizeof(SMTHeader) );
	for(int i = 0; i < header.count; ++i ) {
		// Pull data
		tileData = new unsigned char[tileSize];
		infile.read( (char *)tileData, tileSize);
		if ( header.type == 1 ) {
			tpixels = dxt1_load(tileData, header.res, header.res);
		}
		delete [] tileData;

		for ( int y = 0; y < header.res; y++ ) {
			for ( int x = 0; x < header.res; x++ ) {
				// get pixel locations.
				int tp = (y * header.res + x) * 4;
				int dx = header.res * (i % tileindex_w) + x;
				int dy = header.res * (i / tileindex_w) + y;
				int dp = (dy * xres + dx) * 4;

				memcpy( (char *)&pixels[dp], (char *)&tpixels[tp], 4 );
			}
		}
		delete [] tpixels;
	}
	infile.close();
	imageOutput->open( filename, *imageSpec );
	imageOutput->write_image( TypeDesc::UINT8, pixels);
	imageOutput->close();
	delete imageOutput;
	delete imageSpec;
	delete [] pixels;
	return false;
}

bool
SMT::decompileReconstruct(string prefix)
{
	// OpenImageIO
	ImageOutput *imageOutput = NULL;
	ImageSpec *imageSpec = NULL;
	unsigned char *pixels;
	unsigned char *tpixels;
	unsigned char *tileData;
	ifstream infile;

	// allocate enough data for our large image
	pixels = new unsigned char[tileIndex_w * 32 * tileIndex_l * 32 * 4];

	// Loop through tile index
	infile.open( input_fn.c_str(), ifstream::in );
	for( int ty = 0; ty < tileIndex_l; ++ty ) {
		for( int tx = 0; tx < tileIndex_w; ++tx ) {
			// allocate data for the tile
			tileData = new unsigned char[tileSize];

			// get the index value	
			int tilenum = tileIndex[ty * tileIndex_w + tx];

			// Pull data from the tile file
			infile.seekg( sizeof(SMTHeader) + tilenum * tileSize );
			infile.read( (char *)tileData, tileSize);
			if ( header.type == 1 )
				tpixels = dxt1_load(tileData, header.res, header.res);
			delete [] tileData;

			// Copy to our large image
			// TODO save scanlines to large image once a row is filled to save memory
			for ( int y = 0; y < header.res; y++ ) {
				for ( int x = 0; x < header.res; x++ ) {
					// get pixel locations.
					int tp = (y * header.res + x) * 4;
					int dx = header.res * tx + x;
					int dy = header.res * ty + y;
					int dp = (dy * tileIndex_w * 32 + dx) * 4;
						memcpy( (char *)&pixels[dp], (char *)&tpixels[tp], 4 );
				}
			}
			delete [] tpixels;
		}
	}
	infile.close();

	char filename[256];
	sprintf(filename, "%s_tiles.jpg", prefix.c_str());

	// Save the large image to disk
	imageOutput = NULL;
	imageOutput = ImageOutput::create(filename);
	if(!imageOutput) return true;

	imageSpec = new ImageSpec(tileIndex_w * 32, tileIndex_l * 32, 4, TypeDesc::UINT8);
	imageOutput->open( filename, *imageSpec );
	imageOutput->write_image( TypeDesc::UINT8, pixels);
	imageOutput->close();
	delete imageOutput;
	delete imageSpec;
	delete [] pixels;

	return false;
}

void
SMT::setType(int type)
{
	this->header.type = type;
	// Calculate the size of the tile data
	// size is the raw format of dxt1 with 4 mip levels
	// 32x32, 16x16, 8x8, 4x4
	// 512  + 128  + 32 + 8 = 680
	if(header.type == 1) {
		int tile_res = header.res;
		for( int i=1; i <= 4; i++) {
			tileSize += tile_res/4 * tile_res/4 * 8;
			tile_res /= 2;
		}
	} else {
		if( !quiet )printf("ERROR: '%i' is not a valid type", type);
		tileSize = -1;
	}
	if( verbose )printf("Tile Data Size: %i\n", tileSize);
	
	return;
}
#endif //ndef __SMT_H
