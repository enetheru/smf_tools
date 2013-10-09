// MapConv.cpp : Defines the entry point for the console application.
//#define WIN32 true

#include <string>
#include <fstream>
#include <vector>

// c headers
#include <math.h>
#include <stdio.h>

// external
#include "tclap/CmdLine.h"
#include <nvtt/nvtt.h>
#include <OpenImageIO/imageio.h>

// local headers
#include "mapfile.h"

using namespace std;
using namespace TCLAP;
OIIO_NAMESPACE_USING

struct tile_crc {
	char d[8];
	char n;
	tile_crc();
};

tile_crc::tile_crc()
{
	n = '\0';
}

struct GrassHeader {
	int size;
	int type;
	int grassPtr;
};

class NVTTOutputHandler: public nvtt::OutputHandler
{
public:
	void
	beginImage( int size, int width, int height, int depth, int face,
				int miplevel);

	bool
	writeData(const void *data, int size);
	struct mipinfo {
		int offset;
		int size;
		int width, height, depth;
		int face;
	};

	vector<mipinfo> mip;

	char buffer[MINIMAP_SIZE+1024];
	int offset;
};

void
NVTTOutputHandler::beginImage( int size, int width, int height, int depth, int face,
				int miplevel)
{
	mipinfo info = {offset, size, width, height, depth, face};
	mip.push_back(info);
	return;
}

bool
NVTTOutputHandler::writeData(const void *data, int size)
{
    // Copy mipmap data
    memcpy( &buffer[offset], data, size );
    offset += size;
    return true;
}

template <class T> T *
map_channels(T *in_d, //input data
	int width, int length, // input width, length
   	int in_c, int out_c, // channels in,out
   	int *map, int *fill); // channel map, fill values

template <class T> T *
interpolate_nearest(T *in_d, //input data
	int in_w, int in_l, int chans, // input width, length, channels
	int out_w, int out_l); // output width and length

template <class T> T *
interpolate_bilinear(T *in_d, //input data
	int in_w, int in_l, int chans, // input width, length, channels
	int out_w, int out_l); // output width and length

unsigned char *
dxt1_load(unsigned char* data, int width, int height);

#ifdef WIN32
int
_tmain( int argc, _TCHAR *argv[] )
#else
int
main( int argc, char **argv )
#endif
{
	bool fail = false;
// Generic Options
//  -o --smf-ofn <string>
//     --smt-ofn <string>
//     --no-smt <bool> Don't build smt file
//  -v --verbose <bool> Display extra output
//  -q --quiet <bool> Supress standard output
//     --decompile <string> map name to decompile
	string smf_ofn = "", smt_ofn = "";
	bool no_smt = false;
	bool verbose = false, quiet = false;
	string decompile_fn = "";
//
//  Map Dimensions
//	-w --width <int> Width in spring map units
//	-l --length <int> Length in spring map units
//	-a --height <float> Height of the map in spring map units
//	-d --depth <float> Depth of the map in spring map units
	int width = 0, length = 0, height = 1, depth = 1;
//
// Additional Tile Files
//     --smt-add <string>
//     --smt-index <image>
	vector<string> smt_filenames;
	string tileindex_fn = "";
//
// Displacement
//	-y --displacement <image>
//	   --lowpass <bool>
//	   --invert <bool>
	string displacement_fn = "";
	bool displacement_lowpass = false, displacement_invert = false;
//
// Terrain Type
//	-t --typemap <image>
	string typemap_fn = "";
//
// Minimap
//     --minimap <image>
	string minimap_fn = "";
//
// Diffuse Image
//	-c --diffuse <image> 
//	   --fast-dxt1 <bool> fast compression for dxt1 of diffuse image
	bool fast_dxt1 = false;
	string diffuse_fn = "";
//
// Metal
//	-m --metalmap <image>
	string metalmap_fn = "";
//
// Grass
//  -g --grassmap <image>
	bool grass = false;
	string grassmap_fn = "";
//
// Features
//	-f --featurelist <text>
	bool featurelist;
	string featurelist_fn = "";

	// Globals //
	/////////////
	MapHeader smf_header;
	MapTileHeader smf_tileheader;
	GrassHeader smf_grassheader;
//	MapFeatureHeader smf_featureheader;
//	MapFeatureStruct smf_featurestruct;
//	TileFileHeader smt_header;
	char magic[16];
	int x, z;

	// Tile Generation
	int tiletotal = 0;
	int tilenum = 0;
	vector<int> smt_tilenums;

	// OpenImageIO
	ImageInput *in = NULL; 
	ImageSpec *spec = NULL;
	int xres, yres, channels;
	unsigned char *uint8_pixels;
	unsigned short *uint16_pixels;
	unsigned int *uint_pixels;

	// NVTT
	nvtt::InputOptions inputOptions;
	nvtt::OutputOptions outputOptions;
	nvtt::CompressionOptions compressionOptions;
	nvtt::Compressor compressor;
	NVTTOutputHandler *outputHandler;

	try {
		// Define the command line object.
		CmdLine cmd( "Converts a series of image and text files to .smf and"
						".smt files used in SpringRTS maps.", ' ', "0.5" );

		// Define a value argument and add it to the command line.
		
		// Generic Options //		
		/////////////////////
		ValueArg<string> arg_smf_ofn(
			"o", "outfilename",
			"Output prefix for the spring map file(.smf)",
			false, "", "string", cmd);

		ValueArg<string> arg_smt_ofn(
			"", "smt-outfilename",
			"Output prefix for the spring map tile file(.smt), will default "
			" to the spring map file name",
			false, "", "string", cmd);

		MultiArg<string> arg_smt_add(
			"", "smt-add",
			"External tile file that will be used for finding tiles. Tiles"
			"not found in this will be saved in a new tile file.",
			false, "filename", cmd );

		ValueArg<string> arg_tileindex_fn(
			"", "smt-index",
			"The index image to use for referencing tiles.",
			false, "", "filename", cmd);

		SwitchArg arg_no_smt(
			"", "no-smt",
			"Don't make spring tile file(smt).",
			cmd, false );

		SwitchArg arg_verbose(
			"v", "verbose",
			"Extra information printed to standard output.",
			cmd, false );

		SwitchArg arg_quiet(
			"q", "quiet",
			"Supress information printing to standard output.",
			cmd, false );

		ValueArg<string> arg_decompile_fn(
			"", "decompile",
			"The smf file to decompile.",
			false, "", "filename", cmd);
		
		// Map Dimensions //
		////////////////////
		ValueArg<int> arg_width(
			"w", "width",
			"The Width of the map in spring map units. Must be multiples of 2",
			false, 0, "int", cmd );

		ValueArg<int> arg_length(
			"l", "length",
			"The Length of the map in spring map units. Must be multiples of 2",
			false, 0, "int", cmd );

		ValueArg<float> arg_depth(
			"d", "depth",
			"The deepest point on the map in spring map units, with zero being sea level.",
			false, 1, "float", cmd );

		ValueArg<float> arg_height(
			"a", "height",
			"The highest point on the map in spring map units, with zero being sea level.",
			false, 1, "float", cmd );

		// Diffuse Image //
		///////////////////
		ValueArg<string> arg_diffuse_fn(
			"c", "diffuse",
			"Image to use for the diffuse or colour of the map.",
			false, "", "filename", cmd );

		SwitchArg arg_fast_dxt1(
			"", "fast-dxt1",
			"Use Fast compression method of dxt1",
			cmd, false);

		// Displacement Image //
		////////////////////////
		ValueArg<string> arg_displacement_fn(
			"y", "displacment",
			"Image to use for vertical displacement of the terrain.",
			false, "", "filename", cmd );

		SwitchArg arg_displacement_invert(
			"", "invert",
			"Invert the meaning of black and white.",
			cmd, false );

		SwitchArg arg_displacement_lowpass(
			"", "lowpass",
			"Lowpass filter smoothing hard edges from 8bit colour.",
			cmd, false );

		// Metal //
		///////////
		ValueArg<string> arg_metalmap_fn(
			"m", "metalmap",
			"Image used for built in resourcing scheme.",
			false, "", "filename", cmd);

		// Terrain Type //
		//////////////////
		ValueArg<string> arg_typemap_fn(
			"t", "typemap",
			"Image to define terrain types.",
			false, "", "filename", cmd );

		// Features //
		//////////////
		ValueArg<string> arg_grassmap_fn(
			"g", "grassmap",
			"Image used to place grass.",
			false, "", "filename", cmd );

		ValueArg<string> arg_featurelist_fn(
			"f", "featurelist",
			"Text file defining type, location, rotation of feature and "
			"decal to paint to diffuse texture.",
			false, "", "filename", cmd );

		// Minimap //
		/////////////
		ValueArg<string> arg_minimap_fn(
			"", "minimap",
			"Image file to use for the minimap",
			false, "", "filename", cmd );

		// Parse the args.
		cmd.parse( argc, argv );

		// Get the value parsed by each arg.
		// Generic Options
		verbose = arg_verbose.getValue();
		quiet = arg_quiet.getValue();
		decompile_fn = arg_decompile_fn.getValue();

		smf_ofn = arg_smf_ofn.getValue(); // SMF Output Filename
		smt_ofn = arg_smt_ofn.getValue(); // SMT Output Filename

		// SMT options
		no_smt = arg_no_smt.getValue();
		smt_filenames = arg_smt_add.getValue(); // SMT Input Filename
		tileindex_fn = arg_tileindex_fn.getValue();

		// Map Dimensions
		width = arg_width.getValue();
		length = arg_length.getValue();
		depth = arg_depth.getValue();
		height = arg_height.getValue();

		// Displacement
		displacement_fn = arg_displacement_fn.getValue();
		displacement_invert = arg_displacement_invert.getValue();
		displacement_lowpass = arg_displacement_lowpass.getValue();

		// Terrain Type
		typemap_fn = arg_typemap_fn.getValue();

		// Minimap
		minimap_fn = arg_minimap_fn.getValue();

		// Diffuse Image
		diffuse_fn = arg_diffuse_fn.getValue();
		fast_dxt1 = arg_fast_dxt1.getValue();

		// Metal Map
		metalmap_fn = arg_metalmap_fn.getValue();

		// Grass Map
		grassmap_fn = arg_grassmap_fn.getValue();

		// Feature List
		featurelist_fn = arg_featurelist_fn.getValue();


	} catch ( ArgException &e ) {
		cerr << "error: " << e.error() << " for arg " << e.argId() << endl;
		exit( -1 );
	}


	// Decompile SMF //
	///////////////////
	
	if( decompile_fn.compare( "" ) ) {
	
	// Perform tests for file validity
	ifstream smf_if(decompile_fn.c_str(), ifstream::in);
	smf_if.read( magic, 16);
	if( !smf_if.good() ) {
		if ( !quiet )printf( "ERROR: Cannot open %s\n", decompile_fn.c_str() ); 
		exit(-1);
	}
	
	// perform test for file format
	if( strcmp(magic, "spring map file") ) {
		if( !quiet )printf("ERROR: %s is not a valid smf file.\n", decompile_fn.c_str() );
		exit(-1);
	}

	smf_if.seekg(0);
	smf_if.read( (char *)&smf_header, sizeof(MapHeader) );
	if( verbose ) {
	printf("INFO: Reading %s\n", decompile_fn.c_str() );
	printf("\t%s: %s\n", smf_header.magic, decompile_fn.c_str() );
	printf("\tVersion: %i\n", smf_header.version );
	printf("\tID: %i\n", smf_header.mapid );

	printf("\tWidth: %i\n", smf_header.mapx / 64 );
	printf("\tLength: %i\n", smf_header.mapy / 64 );
	printf("\tSquareSize: %i\n", smf_header.squareSize );
	printf("\tTexelPerSquare: %i\n", smf_header.texelPerSquare );
	printf("\tTileSize: %i\n", smf_header.tilesize );
	printf("\tMinHeight: %f\n", smf_header.minHeight / 512 );
	printf("\tMaxHeight: %f\n", smf_header.maxHeight / 512 );

	printf("\tHeightMapPtr: %i\n", smf_header.heightmapPtr );
	printf("\tTypeMapPtr: %i\n", smf_header.typeMapPtr );
	printf("\tTilesPtr: %i\n", smf_header.tilesPtr );
	printf("\tMiniMapPtr: %i\n", smf_header.minimapPtr );
	printf("\tMetalMapPtr: %i\n", smf_header.metalmapPtr );
	printf("\tFeaturePtr: %i\n", smf_header.featurePtr );

	printf("\tExtraHeaders: %i\n", smf_header.numExtraHeaders );
	} //fi verbose

	int memloc;
	int header_size;
	int header_type;
	for(int i = 0; i < smf_header.numExtraHeaders; ++i ) {
		memloc = smf_if.tellg();
		smf_if.read( (char *)&header_size, 4);
		smf_if.read( (char *)&header_type, 4);
		smf_if.seekg(memloc);
		if(header_type == 1) {
			smf_if.read( (char *)&smf_grassheader, sizeof(GrassHeader));
			grass = true;
		}
		else if( verbose )printf("WARNING: %i is an unknown header type", i);
		smf_if.seekg( memloc + header_size);
	}


	width = smf_header.mapx / 64;
	length = smf_header.mapy / 64;
	ImageOutput *out;

	// output height map
	if( verbose )printf("INFO: Dumping Height Map\n");
	xres = width * 64 + 1;
	yres = length * 64 + 1;
	channels = 1;
	int data_size = xres * yres * channels;
	
	uint16_pixels = new unsigned short[data_size];

	smf_if.seekg(smf_header.heightmapPtr);
	smf_if.read( (char *)uint16_pixels, data_size * 2);

	const char *filename = "out_displacement.tif";

	out = ImageOutput::create(filename);
	if(!out) exit(-1);

	spec = new ImageSpec( xres, yres, channels, TypeDesc::UINT16);
	out->open( filename, *spec );
	out->write_image( TypeDesc::UINT16, uint16_pixels );
	out->close();

	delete out;
	delete spec;
	delete uint16_pixels;

	// output typemap.
	if( verbose )printf("INFO: Dumping Type Map\n");
	xres = width * 32;
	yres = length * 32;
	channels = 1;
	data_size = xres * yres * channels;
	
	uint8_pixels = new unsigned char[data_size];

	smf_if.seekg(smf_header.heightmapPtr);
	smf_if.read( (char *)uint8_pixels, data_size);

	filename = "out_typemap.tif";

	out = ImageOutput::create(filename);
	if(!out) exit(-1);

	spec = new ImageSpec( xres, yres, channels, TypeDesc::UINT8);
	out->open( filename, *spec );
	out->write_image( TypeDesc::UINT8, uint8_pixels );
	out->close();

	delete out;
	delete spec;
	delete uint8_pixels;

	// Output Minimap
	if( verbose )printf("INFO: Dumping Mini Map\n");
	xres = 1024;
	yres = 1024;
	channels = 4;
	
	unsigned char *temp = new unsigned char[MINIMAP_SIZE];
	uint8_pixels = new unsigned char[xres * yres * channels];

	smf_if.seekg(smf_header.minimapPtr);
	smf_if.read( (char *)temp, MINIMAP_SIZE);

	uint8_pixels = dxt1_load(temp, xres, yres);
	delete [] temp;

	filename = "out_minimap.tif";

	out = ImageOutput::create(filename);
	if(!out) exit(-1);

	spec = new ImageSpec( xres, yres, channels, TypeDesc::UINT8);
	out->open( filename, *spec );
	out->write_image( TypeDesc::UINT8, uint8_pixels );
	out->close();

	delete out;
	delete spec;
	delete uint8_pixels;

	// Output TileIndex
	if( verbose )printf("INFO: Dumping Tile Index\n");
	xres = width * 16;
	yres = length * 16;
	channels = 1;
	
	uint_pixels = new unsigned int[xres * yres * channels];

	smf_if.seekg( smf_header.tilesPtr );
	smf_if.read( (char *)&smf_tileheader, sizeof( MapTileHeader ) );

	if( verbose )printf("INFO: %i SMT files referenced\n", smf_tileheader.numTileFiles);
	
	char smt_filename[256];
	for ( int i=0; i < smf_tileheader.numTileFiles; ++i) {
		smf_if.seekg( (int)smf_if.tellg() + 4 );
		smf_if.getline( smt_filename, 255, '\0' );
		if( verbose )printf( "\t%s\n", smt_filename );
		smt_filenames.push_back(smt_filename);
	}


	smf_if.read( (char *)uint_pixels, xres * yres * channels * 4 );

	filename = "out_tileindex.exr";

	out = ImageOutput::create(filename);
	if(!out) exit(-1);

	spec = new ImageSpec( xres, yres, channels, TypeDesc::UINT);
	out->open( filename, *spec );
	out->write_image( TypeDesc::UINT, uint_pixels );
	out->close();

	delete out;
	delete spec;
	delete uint_pixels;

	// Output Metal Map
	if( verbose )printf("INFO: Dumping Metal Map\n");
	xres = width * 32;
	yres = length * 32;
	channels = 1;
	
	uint8_pixels = new unsigned char[xres * yres * channels];

	smf_if.seekg( smf_header.metalmapPtr );
	smf_if.read( (char *)uint8_pixels, xres * yres * channels );

	filename = "out_metalmap.tif";
	out = ImageOutput::create(filename);
	if(!out) exit(-1);

	spec = new ImageSpec( xres, yres, channels, TypeDesc::UINT8);
	out->open( filename, *spec );
	out->write_image( TypeDesc::UINT8, uint8_pixels );
	out->close();

	delete out;
	delete spec;
	delete uint8_pixels;

	if( grass ) {
		// Output Grass Map
		if( verbose )printf("INFO: Dumping Grass Map\n");
		xres = width * 32;
		yres = length * 32;
		channels = 1;
	
		uint8_pixels = new unsigned char[xres * yres * channels];

		smf_if.seekg( smf_grassheader.grassPtr );
		smf_if.read( (char *)uint8_pixels, xres * yres * channels );

		filename = "out_grassmap.tif";
		out = ImageOutput::create(filename);
		if(!out) exit(-1);

		spec = new ImageSpec( xres, yres, channels, TypeDesc::UINT8);
		out->open( filename, *spec );
		out->write_image( TypeDesc::UINT8, uint8_pixels );
		out->close();

		delete out;
		delete spec;
		delete uint8_pixels;
	}

	if( verbose )printf("INFO: Completed\n");
	exit(0);
	} //fi decompile_fn == ""

	// Test arguments for validity before continuing //
	///////////////////////////////////////////////////
	if( verbose )cout << "\n== Testing Arguments for validity ==\n";
	if ( width < 2 || width > 256 || width % 2 ) {
		cout << "ERROR: Width needs to be multiples of 2 between and including 2 and 64" << endl;
		fail = true;
	}
	if ( length < 2 || length > 256 || length % 2 ) {
		cout << "ERROR: Length needs to be multiples of 2 between and including 2 and 64" << endl;
		fail = true;
	}

	// Test SMT name
	if( !smt_ofn.compare( "" ) ) smt_ofn = smf_ofn;

	// Test smt input files exist
	for( unsigned int i=0; i < smt_filenames.size(); ++i) {
		ifstream smt_if(smt_filenames[i].c_str(), ifstream::in);
		if(!smt_if.good()) {
			if ( !quiet )printf( "ERROR: Cannot open %s\n", smt_filenames[i].c_str() ); 
			fail = true;
		}
		else {
			smt_if.read(magic, 16);
			if( strcmp(magic, "spring tilefile") ) {
				if ( !quiet )printf( "ERROR: %s is not a valid smt\n", smt_filenames[i].c_str() );
				fail = true;
			} else {
				smt_if.seekg(20);
				smt_if.read((char *)&tilenum, 4);
				if( tilenum < 1 ) {
					printf( "ERROR: " ); fail = true;
				}
				else if( verbose )printf( "INFO: " );

				if( verbose )printf( "%s has %i tiles.\n", smt_filenames[i].c_str(), tilenum );
				tiletotal += tilenum;
				smt_tilenums.push_back(tilenum);
			}
			smt_if.close();
		}
	}

	// Test Displacement //
	if( !displacement_fn.compare( "" ) ) {
		if ( verbose ) cout << "INFO: Displacement unspecified.\n\tMap will be flat." << endl;
	} else {
		in = NULL;
		in = ImageInput::create( displacement_fn.c_str() );
		if ( !in ) {
			if ( !quiet )cout << "ERROR: Displacement image missing or invalid format." << endl;
			fail = true;
		}
		delete in;
	}

	// Test Typemap //
	if( !typemap_fn.compare( "" ) ) {
		if ( verbose ) cout << "INFO: Typemap unspecified." << endl;
	} else {
		in = NULL;
		in = ImageInput::create( typemap_fn.c_str() );
		if ( !in ) {
			if ( !quiet )cout << "ERROR: Typemap image missing or invalid format." << endl;
			fail = true;
		}
		delete in;
	}
	
	// Test Minimap //
	if( !minimap_fn.compare( "" ) ) {
		if ( verbose ) cout << "INFO: Minimap unspecified." << endl;
	} else {
		in = NULL;
		in = ImageInput::create( minimap_fn.c_str() );
		if ( !in ) {
			if ( !quiet )cout << "ERROR: Minimap image missing or invalid format." << endl;
			fail = true;
		}
		delete in;
	}

	// Test TileIndex //
	if( !tileindex_fn.compare( "" ) ) {
		if ( verbose ) cout << "INFO: Tileindex unspecified." << endl;
	} else {
		in = NULL;
		in = ImageInput::create( tileindex_fn.c_str() );
		if ( !in ) {
			if ( !quiet )cout << "ERROR: tileindex image missing or invalid format." << endl;
			fail = true;
		} else {
			spec = new ImageSpec;
			in->open( tileindex_fn.c_str(), *spec );
			if( spec->width != width * 16 || spec->height != length ) {
				if ( !quiet )
					cout << "ERROR: tileindex image has incorrect dimensions." << endl;
				fail = true;
			}
			in->close();
			delete in;
			delete spec;
		}
	}

	// Test Diffuse //
	if( !diffuse_fn.compare( "" ) ) {
		no_smt = true;
		if ( verbose ) cout << "INFO: Diffuse unspecified.\n\tSMT file will not be built." << endl;
	} else {
		in = NULL;
		in = ImageInput::create( diffuse_fn.c_str() );
		if ( !in ) {
			if ( !quiet )cout << "ERROR: Diffuse image missing or invalid format." << endl; 
			fail = true;
		}
		delete in;
	}

	// Test Metalmap //
	if( !metalmap_fn.compare( "" ) ) {
		if ( verbose ) cout << "INFO: Metalmap unspecified." << endl;
	} else {
		in = NULL;
		in = ImageInput::create( metalmap_fn.c_str() );
		if ( !in ) {
			if ( !quiet )cout << "ERROR: Metalmap image missing or invalid format." << endl;
			fail = true;
		}
		delete in;
	}

	// Test Grassmap //
	if( !grassmap_fn.compare( "" ) ) {
		if ( verbose ) cout << "INFO: Grassmap unspecified." << endl;
		grass = false;
	} else {
		grass = true;
		in = NULL;
		in = ImageInput::create( grassmap_fn.c_str() );
		if ( !in ) {
			if ( !quiet )cout << "ERROR: Grassmap image missing or invalid format." << endl;
			fail = true;
		}
		delete in;
	}

	// Test Featurelist //
	if( !featurelist_fn.compare( "" ) ) {
		if ( verbose ) cout << "INFO: Featurelist unspecified." << endl;
		featurelist = false;
	} else {
		featurelist = true;
	}

	// test for diffuse or smt
	if(no_smt && !smt_filenames.size() ) {
		if ( !quiet ) cout << "ERROR: Either Diffuse or SMT files need to be specified." << endl;
		fail = true;
	}

	if( fail ) exit(-1);



	// Setup Global variables //
	////////////////////////////
	compressionOptions.setFormat(nvtt::Format_DXT1a);
	if( fast_dxt1 ) compressionOptions.setQuality(nvtt::Quality_Fastest); 
	else compressionOptions.setQuality(nvtt::Quality_Normal); 
	outputOptions.setOutputHeader(false);


	// Build SMT Header //
	//////////////////////
	if( !no_smt) {

	uint_pixels =  new unsigned int[width * 16 * length * 16];

	// SMT file comes before SMF because SMF relies on the tiles and filename
	if( verbose ) printf("\n== Creating %s.smt ==\n", smt_ofn.c_str() );
	ofstream smt_of( (smt_ofn+".smt").c_str(), ios::binary | ios::out);
	TileFileHeader tileFileHeader;

	strcpy( tileFileHeader.magic, "spring tilefile" );
	tileFileHeader.version = 1;
	tileFileHeader.tileSize = 32; 
	tileFileHeader.compressionType = 1; 
	tileFileHeader.numTiles = 0; // we overwrite this later 
	smt_of.write( (char *)&tileFileHeader, sizeof(TileFileHeader) );

	xres = width * 512;
	yres = length * 512;
	channels = 4;

	int tcx = width * 16; // tile count x
	int tcz = length * 16; // tile count z


	// Source tile dimensions
	int stw, stl, stc;
	float stw_f, stl_f;

	int dtw, dtl, dtc = 4; //destination tile dimensions
	dtw = dtl = tileFileHeader.tileSize;

	unsigned char *std, *dtd; // source & destination tile pixel data

	vector<tile_crc> tile_crcs;
	tile_crc crc_t;

	// Load up diffuse image
	if( diffuse_fn.compare( "" ) ) {
		in = NULL;
		in = ImageInput::create( diffuse_fn.c_str() );
		spec = new ImageSpec;
		in->open( diffuse_fn.c_str(), *spec );
	}

	if( in ) {
		stw_f = (float)spec->width / tcx;
		stl_f = (float)spec->height / tcz;
		stw = (int)stw_f;
		stl = (int)stl_f;

		stc = spec->nchannels;


		if( verbose ) {
			printf( "INFO: Loaded %s (%i, %i)%i\n",
				diffuse_fn.c_str(), spec->width, spec->height, spec->nchannels);
			if( stw != dtw || stl != dtl )
				printf( "WARNING: Converting tiles from (%i,%i)%i to (%i,%i)%i\n",
					stw,stl,stc,dtw,dtl,dtc);
		}

		for ( z = 0; z < tcz; z++) {
			// pull scanline data from source image
			uint8_pixels = new unsigned char [ spec->width * stl * spec->nchannels ];
			in->read_scanlines( (int)z * stl_f, (int)z * stl_f + stl, 0, TypeDesc::UINT8, uint8_pixels);

			for ( x = 0; x < tcx; x++) {
				if( verbose ) printf("\033[0GINFO: Processing tile %i of %i, Final Count = %i",
					z * tcx + x + 1, tcx * tcz, tileFileHeader.numTiles );
		
				// copy from the scanline data our tile
				std = new unsigned char [stw * stl * stc];
				for( int i = 0; i < stw; ++i ) // rows
					for( int j = 0; j < stl; ++j ) // columns
						for( int c = 0; c < stc; ++c) {
							std[ (i * stl + j) * stc + c] =
								uint8_pixels[ ((i * spec->width) + (int)(x * stw_f) + j) * stc + c ];
				}
						
				// convert tile
				int map[] = {2,1,0,3};
				int fill[] = {0,0,0,255};
				dtd = map_channels(std, stw, stl, stc, dtc, map, fill);
				dtd = interpolate_bilinear(dtd, stw, stl, dtc, dtw, dtl);

				outputHandler = new NVTTOutputHandler();
				outputOptions.setOutputHandler( outputHandler );

				inputOptions.setTextureLayout( nvtt::TextureType_2D, dtw, dtl );
				inputOptions.setMipmapData( dtd, dtw, dtl );

				compressor.process(inputOptions, compressionOptions,
							outputOptions);

				//copy mipmap to use as checksum: 64bits
				memcpy(&crc_t, &outputHandler->buffer[672] , 8);


				bool match = false;
				unsigned int i; //tile index
				if( !tile_crcs.empty() ) {
					for( i = 0; i < tile_crcs.size(); i++ ) {
						if( !strcmp( (char *)&tile_crcs[i], (char *)&crc_t ) ) {
							match = true;
							break;
						} 
					}
				}
				if( !match ) {
					tile_crcs.push_back(crc_t);
					// write tile to file.
					smt_of.write( (char *)outputHandler->buffer, SMALL_TILE_SIZE );
					tileFileHeader.numTiles +=1;
				}
				// Write index to tilemap
				uint_pixels[z * tcx + x] = i;

				delete [] dtd;
				delete outputHandler;
			}
			delete [] uint8_pixels;
		}
		printf("\n");
		in->close();
		delete in;
	}

	// retroactively fix up the tile count.
	smt_of.seekp( 20);
	smt_of.write( (char *)&tileFileHeader.numTiles, 4);
	smt_of.close();

	// add the tile file to the list of smt's used in this map
	smt_filenames.push_back( smt_ofn.append(".smt") );
	smt_tilenums.push_back( tileFileHeader.numTiles );
	tiletotal += tileFileHeader.numTiles;

	}// if(!no_smt)
	
	// Build SMF header //
	//////////////////////
	if( verbose ) printf("\n== Creating %s.smf ==\n", smf_ofn.c_str() );

	MapHeader header;

	strcpy( header.magic, "spring map file" );
	header.version = 1;
	header.mapid = time( NULL );

	header.mapx = width * 64;
	header.mapy = length * 64;

	// must be these values
	header.squareSize = 8;
	header.texelPerSquare = 8;
	header.tilesize = 32;

	header.minHeight = depth * 512;
	header.maxHeight = height * 512;

	header.numExtraHeaders = 0;


	// we revisit these later to correctly set them.
	//FIXME seems not to work for now. will have to solve.
//	header.heightmapPtr = 0;
//	header.typeMapPtr = 0;
//	header.tilesPtr = 0;
//	header.minimapPtr = 0;
//	header.metalmapPtr = 0;
//	header.featurePtr = 0;

	int offset = sizeof( MapHeader );
	// add size of vegetation extra header
	if(grass)offset += sizeof(GrassHeader);
	header.heightmapPtr = offset;

	// add size of heightmap
	offset += (width * 64 +1) * (length * 64 +1) * 2;
	header.typeMapPtr = offset;

	// add size of typemap
	offset += width * length * 1024;
	header.minimapPtr = offset;

	// add size of minimap
	offset += MINIMAP_SIZE;
	header.tilesPtr = offset;

	// add size of tile information.
	offset += sizeof( MapTileHeader );
	// add up the space needed for additional tile files
	for( unsigned int i = 0; i < smt_filenames.size(); ++i ) {
		offset += 4 + smt_filenames[i].size();
	}

	offset += (int)(smf_ofn + ".smt").size() + 4; // new tilefile name
	offset += width * length  * 1024;  // space needed for tile map
	header.metalmapPtr = offset;
 
	// add size of metalmap
	offset += width * length * 1024;
 
	// add size of grass map
	if(grass)offset += width * length * 256;        
	header.featurePtr = offset;

	if(grass)header.numExtraHeaders++;

	// Write Header to file
	ofstream smf_of( (smf_ofn + ".smf").c_str(), ios::out | ios::binary );
	smf_of.write( (char *)&header, sizeof( MapHeader ) );

	// Extra Headers //
	///////////////////

	// Grass:  Byte offset to start of this header is 80
	if ( grass ) {
		if( verbose ) cout << "INFO: Adding grass extra header." << endl;
		smf_grassheader.size = 12;
		smf_grassheader.type = 1; // MEH_Vegetation
		smf_grassheader.grassPtr = 0; // is written later.
		smf_of.write( (char *)&smf_grassheader, sizeof( GrassHeader ) );
	}

	// Height Displacement //
	/////////////////////////
	if( verbose ) cout << "INFO: Processing and writing displacement." << endl;

	// re-write heightPtr to be current position in output stream	
	header.heightmapPtr = smf_of.tellp();
	smf_of.seekp(56); //position of heightmapPtr
	smf_of.write( (char *)&header.heightmapPtr, 4);
	smf_of.seekp(header.heightmapPtr);
	
	// Dimensions of displacement map.
	xres = width * 64 + 1;
	yres = length * 64 + 1;
	channels = 1;

	// Open image if possible, or create blank one.
	in = NULL;
	in = ImageInput::create( displacement_fn.c_str() );
	if( !in ) {
		if( verbose ) cout << "INFO: Generating flat displacement" << endl;
		spec = new ImageSpec(xres, yres, 1, TypeDesc::UINT16);
		uint16_pixels = new unsigned short[ xres * yres ];
		memset( uint16_pixels, 32767, xres * yres * 2);
	} else {
		if( verbose ) printf( "INFO: Reading %s.\n", displacement_fn.c_str() );
		spec = new ImageSpec;
		in->open( displacement_fn.c_str(), *spec );
		uint16_pixels = new unsigned short [spec->width * spec->height * spec->nchannels ];
		in->read_image( TypeDesc::UINT16, uint16_pixels );
		in->close();
		delete in;
	}

	// Fix the number of channels
	if( spec->nchannels != channels ) {
		int map[] = {0};
		int fill[] = {0};
		uint16_pixels = map_channels(uint16_pixels,
			spec->width, spec->height, spec->nchannels,
			channels, map, fill);
	}

	// Fix the size
	if ( spec->width != xres || spec->height != yres ) {
		if( verbose )
			printf( "WARNING: %s is (%i,%i), wanted (%i, %i), Resampling.\n",
			displacement_fn.c_str(), spec->width, spec->height, xres, yres );
		uint16_pixels = interpolate_bilinear( uint16_pixels,
			spec->width, spec->height, channels,
			xres, yres );
	}

	// If inversion of height is specified in the arguments,
	if ( displacement_invert && verbose )
		cout << "WARNING: Height inversion not implemented yet" << endl;

	// If inversion of height is specified in the arguments,
	if ( displacement_lowpass && verbose )
		cout << "WARNING: Height lowpass filter not implemented yet" << endl;
	
	// write height data to smf.
	smf_of.write( (char *)uint16_pixels, xres * yres * 2 );
	delete [] uint16_pixels;
	delete spec;

	// Typemap //
	/////////////
	if( verbose ) cout << "INFO: Processing and writing typemap." << endl;

	// Re-write the heightPtr to be current position in output stream
	//FIXME dosnt work for some reason?
//	header.typeMapPtr = smf_of.tellp();
//	smf_of.seekp(60); //position of typemapPtr
//	smf_of.write( (char *)&header.typeMapPtr, 4);
//	smf_of.seekp(header.typeMapPtr);

	// Dimensions of Typemap
	xres = width * 32;
   	yres = length * 32;
	channels = 1;

	// Open image if possible, or create blank one.
	in = NULL;
	in = ImageInput::create( typemap_fn.c_str() );
	if( !in ) {
			if( verbose ) cout << "INFO: Generating blank typemap." << endl;
			spec = new ImageSpec(xres, yres, 1, TypeDesc::UINT8);
			uint8_pixels = new unsigned char[xres * yres];
			memset( uint8_pixels, 0, xres * yres);

	} else {
		if( verbose ) printf("INFO: Reading %s\n", minimap_fn.c_str() );
		spec = new ImageSpec;
		in->open( typemap_fn.c_str(), *spec );
		uint8_pixels = new unsigned char [spec->width * spec->height * spec->nchannels ];
		in->read_image( TypeDesc::UINT8, uint8_pixels );
		in->close();
		delete in;
	}

	// Fix the number of channels
	if( spec->nchannels != channels ) {
		int map[] = {1};
		int fill[] = {0};
		uint8_pixels = map_channels(uint8_pixels,
			spec->width, spec->height, spec->nchannels,
			channels, map, fill);
	}
	// Fix the Dimensions
	if ( spec->width != xres || spec->height != yres ) {
		if( verbose )
			printf( "WARNING: %s is (%i,%i), wanted (%i, %i) Resampling.\n",
			typemap_fn.c_str(), spec->width, spec->height, xres, yres);

		uint8_pixels = interpolate_nearest(uint8_pixels,
			spec->width, spec->height, channels,
			xres, yres);	
	}

	// write the data to the smf
	smf_of.write( (char *)uint8_pixels, xres * yres );
	delete [] uint8_pixels;
	delete spec;

	// Minimap //
	/////////////
	if( verbose ) cout << "INFO: Processing and writing minimap." << endl;

	// set the minimapPtr to the current position in the output stream
	header.minimapPtr = smf_of.tellp();
	smf_of.seekp(64); //position of minimapPtr
	smf_of.write( (char *)&header.minimapPtr, 4);
	smf_of.seekp(header.minimapPtr);

	// minimap Dimensions
	xres = 1024;
	yres = 1024;
	channels = 4;

	// Load minimap if possible or try alternatives
	in = NULL;
	in = ImageInput::create( minimap_fn.c_str() );
	if( !in ) {
		if( verbose ) cout << "INFO: Attempting to create minimap from diffuse." << endl;
		minimap_fn.assign(diffuse_fn);
		in = ImageInput::create( minimap_fn.c_str() );
	}
	//FIXME attempt to generate minimap from tile files.

	if( !in ) {
		if( verbose ) cout << "WARNING: Creating blank minimap." << endl;
		spec = new ImageSpec(xres, yres, channels, TypeDesc::UINT8);
		uint8_pixels = new unsigned char[xres * yres * channels];
		memset( uint8_pixels, 127, xres * yres * channels); //grey
	} else {
		if( verbose ) printf("INFO: Reading %s\n", minimap_fn.c_str() );
		spec = new ImageSpec;
		in->open( minimap_fn.c_str(), *spec );
		uint8_pixels = new unsigned char [spec->width * spec->height * spec->nchannels ];
		in->read_image( TypeDesc::UINT8, uint8_pixels );
		in->close();
		delete in;
	}

	// Fix channels
	if( spec->nchannels != channels ) {
		int map[] = {2, 1, 0, 3};
		int fill[] = {0, 0, 0, 255};
		uint8_pixels = map_channels(uint8_pixels,
			spec->width, spec->height, spec->nchannels,
			channels, map, fill);
	}
	// Fix Dimensions
	if ( spec->width != xres || spec->height != yres ) {
		if( verbose )
			printf( "WARNING: %s is (%i,%i), wanted (%i, %i), Resampling.\n",
			minimap_fn.c_str(), spec->width, spec->height,
			xres, yres);

		uint8_pixels = interpolate_bilinear(uint8_pixels,
			spec->width, spec->height, channels,
			xres, yres);
	} 

	// setup DXT1 Compression
	inputOptions.setTextureLayout( nvtt::TextureType_2D, 1024, 1024 );
	compressionOptions.setFormat( nvtt::Format_DXT1 );
	
	if( fast_dxt1 ) compressionOptions.setQuality( nvtt::Quality_Fastest ); 
	else compressionOptions.setQuality( nvtt::Quality_Normal ); 
	outputOptions.setOutputHeader( false );

	inputOptions.setMipmapData( uint8_pixels, 1024, 1024 );

	outputHandler = new NVTTOutputHandler();
	outputOptions.setOutputHandler( outputHandler );
	compressor.process( inputOptions, compressionOptions, outputOptions );

	// Write data to smf
	smf_of.write( outputHandler->buffer, MINIMAP_SIZE );
	delete outputHandler;
	delete [] uint8_pixels;
	delete spec;

	// Map Tile Index //
	////////////////////
	if( verbose ) cout << "INFO: Processing and writing maptile index." << endl;

	// Re-write the tilesPtr to be the current position in the output stream
	header.tilesPtr = smf_of.tellp();
	smf_of.seekp(68); //position of tilesPtr
	smf_of.write( (char *)&header.tilesPtr, 4);
	smf_of.seekp(header.tilesPtr);

	// Write map tile header	
	smf_tileheader.numTileFiles = smt_filenames.size(); 
	smf_tileheader.numTiles = width * length * 256;
	smf_of.write( (char *)&smf_tileheader, sizeof( MapTileHeader ) );

	// Add references to smt's
	for(unsigned int i = 0; i < smt_filenames.size(); ++i) {
		if( verbose )printf( "INFO: Adding %s to smf\n", smt_filenames[i].c_str() );
		smf_of.write( (char *)&smt_tilenums[i], 4);
		smf_of.write( smt_filenames[i].c_str(), smt_filenames[i].size() +1 );
	}

	xres = width * 16;
	yres = length * 16;
	channels = 1;

	// Open image if possible, or create blank one.
	in = NULL;
	in = ImageInput::create( tileindex_fn.c_str() );
	if( !in ) {
		//if we have generated a tilemap when building the smt
		if( uint_pixels ) {
			if( verbose ) printf( "INFO: Using generated tile index\n" );
			// Write tile index
			smf_of.write( (char *)uint_pixels, width * 16 * length * 16 * 4);
		} else { //generate a really generic one
			if( verbose ) printf( "INFO: Generating basic tile index\n" );
			int i;
			for ( int z = 0; z < length * 16; z++ )
				for ( int x = 0; x < width * 16; x++) {
					i = (x + z * width * 16) % tiletotal;
					smf_of.write( (char *)&i, sizeof( int ) );
			}
		}
	} else {
		if( verbose ) printf( "INFO: Reading %s.\n", tileindex_fn.c_str() );
		spec = new ImageSpec;
		in->open( tileindex_fn.c_str(), *spec );
		uint_pixels = new unsigned int[ spec->width * spec->height * spec->nchannels ];
		in->read_image( TypeDesc::UINT, uint_pixels );
		in->close();
		smf_of.write( (char *)uint_pixels, xres * yres * 4);
		delete in;
		delete spec;
	}	
	delete [] uint_pixels;

	// Metal Map //
	///////////////
	if( verbose ) cout << "INFO: Processing and writing metalmap." << endl;

	// Re-write the metalmapPtr to be current position in the output stream	
	header.metalmapPtr = smf_of.tellp();
	smf_of.seekp(72); //position of metalmapPtr
	smf_of.write( (char *)&header.metalmapPtr, 4);
	smf_of.seekp(header.metalmapPtr);

	// Dimensions of map
	xres = width * 32;
	yres = length * 32;
	channels = 1;

	// open image if possible or create blank one.
	in = NULL;
	in = ImageInput::create( metalmap_fn.c_str() );
	if( !in ) {
		if( verbose ) cout << "INFO: Creating blank metalmap." << endl;
		spec = new ImageSpec(xres, yres, channels, TypeDesc::UINT8);
		uint8_pixels = new unsigned char[xres * yres * channels];
		memset( uint8_pixels, 0, xres * yres * channels);
	} else {
		if( verbose ) printf("INFO: Reading %s\n", metalmap_fn.c_str() );
		spec = new ImageSpec;
		in->open( metalmap_fn.c_str(), *spec );
		uint8_pixels = new unsigned char [spec->width * spec->height * spec->nchannels ];
		in->read_image( TypeDesc::UINT8, uint8_pixels );
		in->close();
		delete in;
	}

	// Fix the channels
	if( spec->nchannels != channels ) {
		int map[] = {0};
		int fill[] = {0};
		uint8_pixels = map_channels( uint8_pixels,
			spec->width, spec->height, spec->nchannels,
			channels, map, fill );
	}
	// Fix the dimensions
	if ( spec->width != xres || spec->height != yres ) {
		if( verbose )
			printf( "WARNING: %s is (%i,%i), wanted (%i, %i), Resampling.\n",
			metalmap_fn.c_str(), spec->width, spec->height,
			xres, yres );
		uint8_pixels = interpolate_nearest(uint8_pixels,
			spec->width, spec->height, channels,
			xres, yres );
	}

	// Write the data to the smf
	smf_of.write( (char *)uint8_pixels, xres * yres );
	delete [] uint8_pixels;
	delete spec;

	// Vegetation //
	////////////////
	if( grass ) {
		if( verbose ) cout << "INFO: Processing and writing grass map." << endl;

		// Set grassPtr to current position in output stream
		smf_grassheader.grassPtr = smf_of.tellp();
		smf_of.seekp(88); //position of grassPtr
		smf_of.write( (char *)&smf_grassheader.grassPtr, 4);
		smf_of.seekp(smf_grassheader.grassPtr);

		// Dimensions of Grassmap
		xres = width * 32;
		yres = length * 32;
		channels = 1;

		// Open image
		in = NULL;
		in = ImageInput::create( grassmap_fn.c_str() );

		if( verbose ) printf("INFO: Reading %s\n", grassmap_fn.c_str() );
		spec = new ImageSpec;
		in->open( grassmap_fn.c_str(), *spec );
		uint8_pixels = new unsigned char [spec->width * spec->height * spec->nchannels ];
		in->read_image( TypeDesc::UINT8, uint8_pixels );
		in->close();
		delete in;

		// Fix channels
		if( spec->nchannels != channels ) {
			int map[] = {0};
			int fill[] = {0};
			uint8_pixels = map_channels(uint8_pixels,
				spec->width, spec->height, spec->nchannels,
				channels, map, fill);
		}
		// Fix Dimensions
		if ( spec->width != xres || spec->height != yres ) {
			if( verbose )
				printf( "WARNING: %s is (%i,%i), wanted (%i, %i), Resampling.\n",
				grassmap_fn.c_str(), spec->width, spec->height,
				xres, yres);
			uint8_pixels = interpolate_nearest(uint8_pixels,
				spec->width, spec->height, channels,
				xres, yres);
		}

		// Write data to smf
		smf_of.write( (char *)uint8_pixels, xres * yres );
		delete [] uint8_pixels;
		delete spec;
	}

	// Process Features //
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

	smf_of.close();
	return 0;
}

/*
static void
HeightMapFilter( int mapx, int mapy)
{
	float *heightmap2;
	int x,y,x2,y2,dx,dy;
	float h,tmod,mod;

	printf( "Applying lowpass filter to height map\n" );
	heightmap2 = heightmap;
	heightmap = new float[ mapx * mapy ];
	for ( y = 0; y < mapy; ++y )
		for ( x = 0; x < mapx; ++x ) {
			h = 0;
			tmod = 0;
			for ( y2 = max( 0, y - 2 ); y2 < min( mapy, y + 3 ); ++y2 ) {
				dy = y2 - y;
				for ( x2 = max( 0, x - 2 ); x2 < min( mapx, x + 3 ); ++x2 ) {
					dx = x2 - x;
					mod = max( 0.0f,
						1.0f - 0.4f * sqrtf( float( dx * dx + dy * dy ) ) );
					tmod += mod;
					h += heightmap2[ y2 * mapx + x2 ] * mod;
				}
			}
			heightmap[ y * mapx + x ] = h / tmod;
		}
	delete[] heightmap2;
}

*/

template <class T> T *
map_channels(T *in_d, //input data
	int width, int length, // input width, length
   	int in_c, int out_c, // channels in,out
   	int *map, int *fill) // channel map, fill values
{
	T *out_d = new T[width * length * out_c];

	for (int y = 0; y < length; ++y ) // rows
		for ( int x = 0; x < width; ++x ) // columns
			for ( int c = 0; c < out_c; ++c ) { // output channels
				if( map[ c ] >= in_c )
					out_d[ (y * width + x) * out_c + c ] = fill[ c ];
				else
					out_d[ (y * width + x) * out_c + c ] =
						in_d[ (y * width + x) * in_c + map[ c ] ];
	}

	delete [] in_d;
	return out_d;
}

template <class T> T *
interpolate_nearest(T *in_d, //input data
	int in_w, int in_l, int chans, // input width, length, channels
	int out_w, int out_l) // output width and length
{
	int in_x, in_y; // origin coordinates

	T *out_d = new T[out_w * out_l * chans];

	for (int y = 0; y < out_l; ++y ) // output rows
		for ( int x = 0; x < out_w; ++x ) { // output columns
			in_x = (float)x / out_w * in_w; //calculate the input coordinates
			in_y = (float)y / out_l * in_l;

			for ( int c = 0; c < chans; ++c ) // output channels
				out_d[ (y * out_w + x) * chans + c ] =
					in_d[ (in_y * in_w + in_x) * chans + c ];
	}

	delete [] in_d;
	return out_d;
}

template <class T> T *
interpolate_bilinear(T *in_d, //input data
	int in_w, int in_l, int chans, // input width, length, channels
	int out_w, int out_l) // output width and length
{
	float in_x, in_y; // sub pixel coordinates on source map.
	// the values of the four bounding texels + two interpolated ones.
	T p1,p2,p3,p4,p5,p6,p7; 
	int x1,x2,y1,y2;

	T *out_d = new T[out_w * out_l * chans];

	for (int y = 0; y < out_l; ++y ) // output rows
		for ( int x = 0; x < out_w; ++x ) { // output columns
			in_x = (float)x / (out_w-1) * (in_w-1); //calculate the input coordinates
			in_y = (float)y / (out_l-1) * (in_l-1);

			x1 = floor(in_x);
			x2 = ceil(in_x);
			y1 = floor(in_y);
			y2 = ceil(in_y);
			for ( int c = 0; c < chans; ++c ) { // output channels
				p1 = in_d[ (y1 * in_w + x1) * chans + c ];
				p2 = in_d[ (y1 * in_w + x2) * chans + c ];
				p3 = in_d[ (y2 * in_w + x1) * chans + c ];
				p4 = in_d[ (y2 * in_w + x2) * chans + c ];
				p5 = p2 * (in_x - x1) + p1 * (x2 - in_x);
				p6 = p4 * (in_x - x1) + p3 * (x2 - in_x);
				if(x1 == x2){ p5 = p1; p6 = p3; }
				p7 = p6 * (in_y - y1) + p5 * (y2 - in_y);
				if(y1 == y2) p7 = p5;
				out_d[ (y * out_l + x) * chans + c] = p7;

			}
	}

	delete [] in_d;
	return out_d;
}

#define RM	0x0000F800
#define GM  0x000007E0
#define BM  0x0000001F

#define RED_RGB565(x) ((x&RM)>>11)
#define GREEN_RGB565(x) ((x&GM)>>5)
#define BLUE_RGB565(x) (x&BM)
#define PACKRGB(r, g, b) (((r<<11)&RM) | ((g << 5)&GM) | (b&BM) )

unsigned char *
dxt1_load(unsigned char* data, int width, int height)
{
	unsigned char *out_d = new unsigned char[width * height * 4];
	memset( out_d, 0, width * height * 4 );

	int numblocks = ( (width + 3) / 4) * ((height + 3) / 4);

	for ( int i = 0; i < numblocks; i++ ) {

		unsigned short color0 = *(unsigned short *)&data[0];
		unsigned short color1 = *(unsigned short *)&data[2];

		int r0 = RED_RGB565( color0 ) << 3;
		int g0 = GREEN_RGB565( color0 ) << 2;
		int b0 = BLUE_RGB565( color0 ) << 3;

		int r1 = RED_RGB565( color1 ) << 3;
		int g1 = GREEN_RGB565( color1 ) << 2;
		int b1 = BLUE_RGB565( color1 ) << 3;

		unsigned int bits = *(unsigned int*)&data[4];

		for ( int a = 0; a < 4; a++ ) {
			for ( int b = 0; b < 4; b++ ) {
				int x = 4 * (i % ((width + 3) / 4)) + b;
				int y = 4 * (i / ((width + 3) / 4)) + a;
				bits >>= 2;
				unsigned char code = bits & 0x3;

				if ( color0 > color1 ) {
					if ( code == 0 ) {
						out_d[ (y * width + x) * 4 + 0 ] = r0;
						out_d[ (y * width + x) * 4 + 1 ] = g0;
						out_d[ (y * width + x) * 4 + 2 ] = b0;
					}
					else if ( code == 1 ) {
						out_d[ (y * width + x ) * 4 + 0 ] = r1;
						out_d[ (y * width + x ) * 4 + 1 ] = g1;
						out_d[ (y * width + x ) * 4 + 2 ] = b1;
					}
					else if ( code == 2 ) {
						out_d[ (y * width + x) * 4 + 0 ] = (r0 * 2 + r1) / 3;
						out_d[ (y * width + x) * 4 + 1 ] = (g0 * 2 + g1) / 3;
						out_d[ (y * width + x) * 4 + 2 ] = (b0 * 2 + b1) / 3;
					} else {	
						out_d[ (y * width + x) * 4 + 0 ] = (r0 + r1 * 2) / 3;
						out_d[ (y * width + x) * 4 + 1 ] = (g0 + g1 * 2) / 3;
						out_d[ (y * width + x) * 4 + 2 ] = (b0 + b1 * 2) / 3;
					}
				} else {
					if ( code == 0 ) {
						out_d[ (y * width + x) * 4 + 0 ] = r0;
						out_d[ (y * width + x) * 4 + 1 ] = g0;
						out_d[ (y * width + x) * 4 + 2 ] = b0;
					}
					else if ( code == 1 ) {
						out_d[ (y * width + x) * 4 + 0 ] = r1;
						out_d[ (y * width + x) * 4 + 1 ] = g1;
						out_d[ (y * width + x) * 4 + 2 ] = b1;
					}
					else if ( code == 2 ) {
						out_d[ (y * width + x) * 4 + 0 ] = (r0 + r1) / 2;
						out_d[ (y * width + x) * 4 + 1 ] = (g0 + g1) / 2;
						out_d[ (y * width + x) * 4 + 2 ] = (b0 + b1) / 2;
					} else {	
						out_d[ (y * width + x) * 4 + 0] = 0;
						out_d[ (y * width + x) * 4 + 1] = 0;
						out_d[ (y * width + x) * 4 + 2] = 0;
					}
				}
			}
		}
		data += 8;
	}
	return out_d;
}
