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

	char buffer[MINIMAP_SIZE+1024];
	int offset;
};

void
NVTTOutputHandler::beginImage( int size, int width, int height, int depth, int face,
				int miplevel)
{
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

unsigned short *
uint16_filter_bilinear(unsigned short *in_data,
	int in_w, int in_l, int in_c,
	int out_w, int out_l);

unsigned short *
uint16_filter_trilinear(unsigned short *in_data,
	int in_w, int in_l, int in_c,
	int out_w, int out_l);

template <class T> T *
map_channels(T *in_d, //input data
	int width, int length, // input width, length
   	int in_c, int out_c, // channels in,out
   	int *map, int *fill); // channel map, fill values

template <class T> T *
interpolate_nearest(T *in_d, //input data
	int in_w, int in_l, int in_c, // input width, length, channels
	int out_w, int out_l); // output width and length

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
	string smf_ofn = "", smt_ofn = "";
	bool no_smt = false;
	bool verbose = false, quiet = false;
//
//  Map Dimensions
//	-w --width <int> Width in spring map units
//	-l --length <int> Length in spring map units
//	-a --height <float> Height of the map in spring map units
//	-d --depth <float> Depth of the map in spring map units
	int width = 0, length = 0, height = 1, depth = 1;
//
// Additional Tile Files
//     --smt-in <string>
	vector<string> smt_in;
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
	bool grass;
	string grassmap_fn = "";
//
// Features
//	-f --featurelist <text>
	bool featurelist;
	string featurelist_fn = "";

	// Variables I use a lot!! //
	///////////////////////////////////
	char magic[16];
	ImageInput *in = NULL; 
	ImageSpec *spec = NULL;
	int xres, zres, channels;
	int x, z;
	unsigned short *uint16_pixels;
	unsigned char *uint8_pixels;
	int tiletotal = 0;
	int tilenum = 0;
	vector<int> smt_tilenums;

	// setup NVTT compression variables.
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
			true, "", "string", cmd);

		ValueArg<string> arg_smt_ofn(
			"", "smt-outfilename",
			"Output prefix for the spring map tile file(.smt), will default "
			" to the spring map file name",
			false, "", "string", cmd);

		MultiArg<string> arg_smt_in(
			"", "smt-in",
			"External tile file that will be used for finding tiles. Tiles"
			"not found in this will be saved in a new tile file.",
			false, "filename", cmd );

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
		
		// Map Dimensions //
		////////////////////
		ValueArg<int> arg_width(
			"w", "width",
			"The Width of the map in spring map units. Must be multiples of 2",
			true, 8, "int", cmd );

		ValueArg<int> arg_length(
			"l", "length",
			"The Length of the map in spring map units. Must be multiples of 2",
			true, 8, "int", cmd );

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

		smf_ofn = arg_smf_ofn.getValue(); // SMF Output Filename
		smt_ofn = arg_smt_ofn.getValue(); // SMT Output Filename

		// Additional SMT Files
		no_smt = arg_no_smt.getValue();
		smt_in = arg_smt_in.getValue(); // SMT Input Filename

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
	for( unsigned int i=0; i < smt_in.size(); ++i) {
		ifstream smt_if(smt_in[i].c_str(), ifstream::in);
		if(!smt_if.good()) {
			if ( !quiet )printf( "ERROR: Cannot open %s\n", smt_in[i].c_str() ); 
			fail = true;
		}
		else {
			smt_if.read(magic, 16);
			if( strcmp(magic, "spring tilefile") ) {
				if ( !quiet )printf( "ERROR: %s is not a valid smt\n", smt_in[i].c_str() );
				fail = true;
			} else {
				smt_if.seekg(20);
				smt_if.read((char *)&tilenum, 4);
				if( tilenum < 1 ) {
					printf( "ERROR: " ); fail = true;
				}
				else if( verbose )printf( "INFO: " );

				if( verbose )printf( "%s has %i tiles.\n", smt_in[i].c_str(), tilenum );
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
		in = ImageInput::create( minimap_fn.c_str() );
		if ( !in ) {
			if ( !quiet )cout << "ERROR: Minimap image missing or invalid format." << endl;
			fail = true;
		}
		delete in;
	}

	// Test Diffuse //
	if( !diffuse_fn.compare( "" ) ) {
		no_smt = true;
		if ( verbose ) cout << "INFO: Diffuse unspecified.\n\tSMT file will not be built." << endl;
	} else {
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

	if(no_smt && !smt_in.size() ) {
		if ( !quiet ) cout << "ERROR: Either Diffuse or SMT files need to be specified." << endl;
		fail = true;
	}

	in = NULL;
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
	zres = length * 512;
	channels = 4;

	int tcx = width * 16; // tile count x
	int tcz = length * 16; // tile count z

	int stw, stl, stc; // source tile dimensions
	int dtw, dtl, dtc = 4; //destination tile dimensions
	dtw = dtl = tileFileHeader.tileSize;

	unsigned char *std, *dtd; // source & destination tile pixel data

	outputHandler = new NVTTOutputHandler();
	outputOptions.setOutputHandler( outputHandler );

	// Load up diffuse image
	if( diffuse_fn.compare( "" ) ) {
		in = ImageInput::create( diffuse_fn.c_str() );
		spec = new ImageSpec;
		in->open( diffuse_fn.c_str(), *spec );
	}

	if( in ) {
		stw = spec->width / tcx;
		stl = spec->height / tcz;
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
			in->read_scanlines(z * stl, z * stl + stl, 0, TypeDesc::UINT8, uint8_pixels);

			for ( x = 0; x < tcx; x++) {
				if( verbose ) printf("\033[0GINFO: Processing tile %i of %i",
							z * tcx + x + 1, tcx * tcz);
		
				// copy from the scanline data our tile
				std = new unsigned char [stw * stl * stc];
				for( int i = 0; i < stw; ++i ) // rows
					for( int j = 0; j < stl; ++j ) // columns
						for( int c = 0; c < stc; ++c) {
							std[ (i * stl + j) * stc + c] =
								uint8_pixels[ ((i * spec->width) + (x * stw) + j) * stc + c ];
				}
						
				// convert tile
				int map[] = {2,1,0,3};
				int fill[] = {0,0,0,255};
//				printf("map channels\n");
				dtd = map_channels(std, stw, stl, stc, dtc, map, fill);
//				printf("interpolate nearest\n");
				dtd = interpolate_nearest(dtd, stw, stl, dtc, dtw, dtl);

				outputHandler->offset=0;
				inputOptions.setTextureLayout( nvtt::TextureType_2D, dtw, dtl );
				inputOptions.setMipmapData( dtd, dtw, dtl );
				compressor.process(inputOptions, compressionOptions,
							outputOptions);
				smt_of.write( (char *)outputHandler->buffer, SMALL_TILE_SIZE );
				// write tile to file.
				tileFileHeader.numTiles +=1;
				delete [] dtd;
			}
			delete [] uint8_pixels;
		}
		printf("\n");
		in->close();
		delete in;
		delete outputHandler;
	}

	smt_of.seekp( 20);
	smt_of.write( (char *)&tileFileHeader.numTiles, 4);
	smt_of.close();

	// add the tile file to the list of smt's used in this map
	smt_in.push_back( smt_ofn.append(".smt") );
	smt_tilenums.push_back( tileFileHeader.numTiles );
	tiletotal += tileFileHeader.numTiles;

	}// if(!no_smt)
	
/*	ifstream ifs;
	int numNamedFeatures = 0;

	ifs.open( featureListFile.c_str(), ifstream::in );
	int i = 0;
	rotations = new short int[ 65000 ];
	while ( ifs.good() ) {
		char c[ 100 ] = "";
		ifs.getline( c, 100 );
		string tmp = c;
		int l = tmp.find( ' ' );

		if( l > 0 ) rotations[ i ] = atoi( tmp.substr( l + 1 ).c_str() );
		else rotations[ i ] = 0;

		F_Spec.push_back( tmp.substr( 0 , l ).c_str() );
		numNamedFeatures++;
	}
	ifs.close();
	
	
	ifstream ifp;
	ifp.open( featurePlaceFile.c_str(), ifstream::in );
	// we are gonna store the features from the lua file in this struct and
	// then pass this over to the featurecreator.
	vector<LuaFeature> extrafeatures; 
	int lnum = 0;
	while( ifp.good() ) {
		lnum++;
		char c[ 255 ] = "";
		ifp.getline( c, 255 );
		string tmp = c;
		int x = tmp.find( "name" );
		if ( x > 0 ) {
			// stupid lua file specifies a goddamned feature.
			// now to parse this shit, example line:
			//	{ name = 'agorm_talltree6', x = 224, z = 3616, rot = "0" },
			int s = tmp.find( '\'', x );
			string name = tmp.substr( s +1, tmp.find( '\'', s +1 ) -s -1 );
			//printf( "%s", name );
			string rot = "";
			int posx = 0;
			int posz = 0;
			short int irot = -1;

			x = tmp.find( "rot" );
			if ( x >-1 ) {
				s = tmp.find( '\"', x );
				rot= tmp.substr( s +1, tmp.find( '\"', s +1 ) -s -1 );
				//printf( "%s", rot );
				int ttt = rot.find( "south" );
				if ( ttt > -1 ) irot = 0;
				else if ( rot.find( "east"  ) != string::npos ) irot =  16384;
				else if ( rot.find( "north" ) != string::npos ) irot = -32768;
				else if ( rot.find( "west"  ) != string::npos ) irot = -16384;
				else irot = atoi( rot.c_str() );
			} else {
				printf( "failed to find rot in feature placer file on line:"
						"%s line num: %i \n", tmp.c_str(), lnum );
				continue;
			}

			x = tmp.find( "x =" );
			if ( x > -1 ) {
				s = tmp.find( ',', x );
				string sposx = tmp.substr( x + 4, s - x + 4 );
				//printf( "%s", sposx );
				posx = atoi( sposx.c_str() );
				if ( posx == 0 ) printf( "Suspicious 0 value for pos X read"
					"from line: %s (devs: atoi returned 0) line num:%i \n",
					tmp.c_str(), lnum );
			} else {
				printf( "failed to find x = in feature placer file on line:"
					"%s line num:%i \n", tmp.c_str(), lnum );
				continue;
			}
			
			x = tmp.find( "z =" );
			if ( x > -1 ) {
				s = tmp.find( ',', x );
				string sposz = tmp.substr( x + 4, s - x + 4 );
				//printf( "%s", sposz );
				posz = atoi( sposz.c_str() );
				if ( posz == 0 ) printf( "Suspicious 0 value for pos Z read"
					"from line: %s (devs: atoi returned 0) line num:%i \n",
					tmp.c_str(), lnum );
			} else {
				printf( "failed to find z = in feature placer file on line:"
					"%s line num: %i \n", tmp.c_str(), lnum );
				continue;
			}

			bool found = false;
			unsigned int j;
			for ( j = 0; j < F_Spec.size(); j++ )
				if ( F_Spec[j].compare( name ) == 0 ) found = true;

			if ( !found ) {
				F_Spec.push_back( name );
				printf("New feature name from feature placement file: %s"
					"line num:%i\n", name.c_str(), lnum );
			} 
			LuaFeature lf;
			lf.name = name;
			lf.posx = posx;
			lf.posz = posz;
			lf.rot = irot;
			extrafeatures.push_back( lf );
		}
	}

	featureCreator.CreateFeatures( &tileHandler.bigTex, 0, 0,
		numNamedFeatures, feature_fn, geoVentFile, extrafeatures, F_Spec);
*/

	// Build SMF header //
	//////////////////
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


	// we revisit these later to correctly set them.
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
	for( unsigned int i = 0; i < smt_in.size(); ++i ) offset += 4 + smt_in[i].size();

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
	GrassHeader grassHeader;

	// Grass:  Byte offset to start of this header is 80
	if ( grass ) {
		if( verbose ) cout << "INFO: Adding grass extra header." << endl;
		grassHeader.size = 12;
		grassHeader.type = 1; // MEH_Vegetation
		grassHeader.grassPtr = 0; // is written later.
		smf_of.write( (char *)&grassHeader, sizeof( GrassHeader ) );
	}

	// Height Displacement //
	/////////////////////////
	if( verbose ) cout << "INFO: Processing and writing displacement." << endl;
	
	header.heightmapPtr = smf_of.tellp();
	smf_of.seekp(56); //position of heightmapPtr
	smf_of.write( (char *)&header.heightmapPtr, 4);
	smf_of.seekp(header.heightmapPtr);
	
	in = NULL;
	xres = width * 64 + 1;
	zres = length * 64 + 1;
	channels = 1;
	int ox, oz;
	unsigned short *displacement;

	in = ImageInput::create( displacement_fn.c_str() );
	if( !in ) {
		if( verbose ) cout << "INFO: Generating flat displacement" << endl;
		spec = new ImageSpec(xres, zres, 1, TypeDesc::UINT16);
		uint16_pixels = new unsigned short[ xres * zres ];
		memset( uint16_pixels, 32767, xres * zres * 2);
	} else {
		if( verbose ) printf( "INFO: Reading %s.\n", displacement_fn.c_str() );
		spec = new ImageSpec;
		in->open( displacement_fn.c_str(), *spec );
		uint16_pixels = new unsigned short [spec->width * spec->height * spec->nchannels ];
		in->read_image( TypeDesc::UINT16, uint16_pixels );
		in->close();
		delete in;
	}

	// If the image isnt the correct timensions or not the right number of
	// channels, resample using nearest.
	if (   spec->width     != xres
		|| spec->height    != zres
		|| spec->nchannels != channels ) {
		if( verbose )
			printf( "WARNING: %s is (%i,%i)%i, wanted (%i, %i)1 Resampling.\n",
			displacement_fn.c_str(), spec->width, spec->height, spec->nchannels,
			xres, zres);

//		displacement = uint16_filter_trilinear(uint16_pixels,
//			spec->width, spec->height, spec->nchannels, xres, zres);
		int map[] = {0};
		int fill[] = {0};
		uint16_pixels = map_channels(uint16_pixels,
			spec->width, spec->height, spec->nchannels,
			channels, map, fill);

		// resample nearest
		displacement = new unsigned short[ xres * zres ];
		for (int z = 0; z < zres; z++ ) // rows
		for ( int x = 0; x < xres; x++ ) { // columns
				ox = (float)x / xres * spec->width;
				oz = (float)z / zres * spec->height;
				displacement[ z * xres + x ] =
					uint16_pixels[ oz * spec->width + ox];
		}
		delete [] uint16_pixels;
	} else displacement = uint16_pixels;

	// If inversion of height is specified in the arguments,
	if ( displacement_invert && verbose )
		cout << "WARNING: Height inversion not implemented yet" << endl;

	// If inversion of height is specified in the arguments,
	if ( displacement_lowpass && verbose )
		cout << "WARNING: Height lowpass filter not implemented yet" << endl;
	
	// write height data to smf.
	smf_of.write( (char *)displacement, xres * zres * 2 );
	delete [] displacement;
	delete spec;

	// Typemap //
	/////////////
	if( verbose ) cout << "INFO: Processing and writing typemap." << endl;

//	header.typeMapPtr = smf_of.tellp();
//	smf_of.seekp(60); //position of typemapPtr
//	smf_of.write( (char *)&header.typeMapPtr, 4);
//	smf_of.seekp(header.typeMapPtr);

	in = NULL;
	xres = width * 32;
   	zres = length * 32;
	channels = 1;
	unsigned char *typemap;

	in = ImageInput::create( typemap_fn.c_str() );
	if( !in ) {
			if( verbose ) cout << "INFO: Generating blank typemap." << endl;
			spec = new ImageSpec(xres, zres, 1, TypeDesc::UINT8);
			uint8_pixels = new unsigned char[xres * zres];
			memset( uint8_pixels, 0, xres * zres);

	} else {
		if( verbose ) printf("INFO: Reading %s\n", minimap_fn.c_str() );
		spec = new ImageSpec;
		in->open( typemap_fn.c_str(), *spec );
		uint8_pixels = new unsigned char [spec->width * spec->height * spec->nchannels ];
		in->read_image( TypeDesc::UINT8, uint8_pixels );
		in->close();
		delete in;
	}

	// If the image isnt the correct timensions or not the right number of
	// channels, resample using nearest.
	if (   spec->width     != xres
		|| spec->height    != zres
		|| spec->nchannels != 1 ) {
		if( verbose )
			printf( "WARNING: %s is (%i,%i)%i, wanted (%i, %i)1 Resampling.\n",
			typemap_fn.c_str(), spec->width, spec->height, spec->nchannels,
			xres, zres);

		// convert image
		int map[] = {1};
		int fill[] = {0};
		uint8_pixels = map_channels(uint8_pixels,
			spec->width, spec->height, spec->nchannels,
			channels, map, fill);
		typemap = interpolate_nearest(uint8_pixels,
			spec->width, spec->height, channels,
			xres, zres);
		
	} else typemap = uint8_pixels;

	smf_of.write( (char *)typemap, xres * zres );
	delete [] typemap;
	delete spec;

	// Minimap //
	/////////////
	if( verbose ) cout << "INFO: Processing and writing minimap." << endl;

	header.minimapPtr = smf_of.tellp();
	smf_of.seekp(64); //position of minimapPtr
	smf_of.write( (char *)&header.minimapPtr, 4);
	smf_of.seekp(header.minimapPtr);

	in = NULL;
	xres = 1024;
	zres = 1024;
	channels = 4;
	unsigned char *minimap;

	in = ImageInput::create( minimap_fn.c_str() );

	if( !in ) {
		if( verbose ) cout << "INFO: Attempting to create minimap from diffuse." << endl;
		minimap_fn.assign(diffuse_fn);
		in = ImageInput::create( minimap_fn.c_str() );
	}

	if( !in ) {
		minimap_fn.assign(displacement_fn);
		in = ImageInput::create( minimap_fn.c_str() );
		if( verbose ) cout << "INFO: Attempting to create minimap from displacement." << endl;
	}

	if( !in ) {
		if( verbose ) cout << "WARNING: Creating blank minimap." << endl;
		spec = new ImageSpec(xres, zres, channels, TypeDesc::UINT8);
		uint8_pixels = new unsigned char[xres * zres * channels];
		memset( uint8_pixels, 0, xres * zres * channels);

	} else {
		if( verbose ) printf("INFO: Reading %s\n", minimap_fn.c_str() );
		spec = new ImageSpec;
		in->open( minimap_fn.c_str(), *spec );
		uint8_pixels = new unsigned char [spec->width * spec->height * spec->nchannels ];
		in->read_image( TypeDesc::UINT8, uint8_pixels );
		in->close();
		delete in;
	}

	// If the image isnt the correct timensions or not the right number of
	// channels, resample using nearest.
	if (   spec->width     != xres
		|| spec->height    != zres
		|| spec->nchannels != channels ) {
		if( verbose )
			printf( "WARNING: %s is (%i,%i)%i, wanted (%i, %i)%i Resampling.\n",
			minimap_fn.c_str(), spec->width, spec->height, spec->nchannels,
			xres, zres, channels);

		// convert image
		int map[] = {2, 1, 0, 3};
		int fill[] = {0, 0, 0, 255};
		uint8_pixels = map_channels(uint8_pixels,
			spec->width, spec->height, spec->nchannels,
			channels, map, fill);
		minimap = interpolate_nearest(uint8_pixels,
			spec->width, spec->height, channels,
			xres, zres);

	} else minimap = uint8_pixels;

	inputOptions.setTextureLayout(nvtt::TextureType_2D, 1024, 1024);
	compressionOptions.setFormat(nvtt::Format_DXT1);
	
	if( fast_dxt1 ) compressionOptions.setQuality(nvtt::Quality_Fastest); 
	else compressionOptions.setQuality(nvtt::Quality_Normal); 
	outputOptions.setOutputHeader(false);

	inputOptions.setMipmapData( minimap, 1024, 1024 );

	outputHandler = new NVTTOutputHandler();
	outputOptions.setOutputHandler( outputHandler );
	compressor.process(inputOptions, compressionOptions,
				outputOptions);

	smf_of.write( outputHandler->buffer, MINIMAP_SIZE );

	delete outputHandler;
	delete [] minimap;
	delete spec;

	// Map Tile Index //
	////////////////////
	if( verbose ) cout << "INFO: Processing and writing maptile index." << endl;
	
	header.tilesPtr = smf_of.tellp();
	smf_of.seekp(68); //position of tilesPtr
	smf_of.write( (char *)&header.tilesPtr, 4);
	smf_of.seekp(header.tilesPtr);
	
	in = NULL;
	MapTileHeader mapTileHeader;
	mapTileHeader.numTileFiles = smt_in.size(); 
	mapTileHeader.numTiles = width * length * 256;
	smf_of.write( (char *)&mapTileHeader, sizeof( MapTileHeader ) );

	// add references to smt's
	for(unsigned int i = 0; i < smt_in.size(); ++i) {
		if( verbose )printf( "INFO: Adding %s to smf\n", smt_in[i].c_str() );
		smf_of.write( (char *)&smt_tilenums[i], 4);
		smf_of.write( smt_in[i].c_str(), smt_in[i].size() +1 );
	}

	// Write tile index
	int i;
	for ( int z = 0; z < length * 16; z++ )
		for ( int x = 0; x < width * 16; x++) {
			i = (x + z * width * 16) % tiletotal;
			smf_of.write( (char *)&i, sizeof( int ) );
	}

	// Metal Map //
	///////////////
	if( verbose ) cout << "INFO: Processing and writing metalmap." << endl;
	
	header.metalmapPtr = smf_of.tellp();
	smf_of.seekp(72); //position of metalmapPtr
	smf_of.write( (char *)&header.metalmapPtr, 4);
	smf_of.seekp(header.metalmapPtr);

	in = NULL;
	xres = width * 32;
	zres = length * 32;
	channels = 1;
	unsigned char *metalmap;

	in = ImageInput::create( metalmap_fn.c_str() );
	if( !in ) {
		if( verbose ) cout << "INFO: Creating blank metalmap." << endl;
		spec = new ImageSpec(xres, zres, channels, TypeDesc::UINT8);
		uint8_pixels = new unsigned char[xres * zres * channels];
		memset( uint8_pixels, 0, xres * zres * channels);
	} else {
		if( verbose ) printf("INFO: Reading %s\n", metalmap_fn.c_str() );
		spec = new ImageSpec;
		in->open( metalmap_fn.c_str(), *spec );
		uint8_pixels = new unsigned char [spec->width * spec->height * spec->nchannels ];
		in->read_image( TypeDesc::UINT8, uint8_pixels );
		in->close();
		delete in;
	}

	// If the image isnt the correct dimensions or not the right number of
	// channels, resample using nearest.
	if (   spec->width     != xres
		|| spec->height    != zres
		|| spec->nchannels != channels ) {
		if( verbose )
			printf( "WARNING: %s is (%i,%i)%i, wanted (%i, %i)%i Resampling.\n",
			metalmap_fn.c_str(), spec->width, spec->height, spec->nchannels,
			xres, zres, channels);

		// convert image
		int map[] = {0};
		int fill[] = {255};
		uint8_pixels = map_channels(uint8_pixels,
			spec->width, spec->height, spec->nchannels,
			channels, map, fill);
		metalmap = interpolate_nearest(uint8_pixels,
			spec->width, spec->height, channels,
			xres, zres);

	} else metalmap = uint8_pixels;

	smf_of.write( (char *)metalmap, xres * zres );
	delete [] metalmap;

	// Vegetation //
	////////////////
	if( grass ) {
		if( verbose ) cout << "INFO: Processing and writing grass map." << endl;

		grassHeader.grassPtr = smf_of.tellp();
		smf_of.seekp(88); //position of grassPtr
		smf_of.write( (char *)&grassHeader.grassPtr, 4);
		smf_of.seekp(grassHeader.grassPtr);

		in = NULL;
		xres = width * 32;
		zres = length * 32;
		channels = 1;
		unsigned char *grassmap;

		in = ImageInput::create( grassmap_fn.c_str() );
		if( in ) {
			if( verbose ) printf("INFO: Reading %s\n", grassmap_fn.c_str() );
			spec = new ImageSpec;
			in->open( grassmap_fn.c_str(), *spec );
			uint8_pixels = new unsigned char [spec->width * spec->height * spec->nchannels ];
			in->read_image( TypeDesc::UINT8, uint8_pixels );
			in->close();
			delete in;
		}

		// If the image isnt the correct dimensions or not the right number of
		// channels, resample using nearest.
		if (   spec->width     != xres
			|| spec->height    != zres
			|| spec->nchannels != channels ) {
			if( verbose )
				printf( "WARNING: %s is (%i,%i)%i, wanted (%i, %i)%i Resampling.\n",
				grassmap_fn.c_str(), spec->width, spec->height, spec->nchannels,
				xres, zres, channels);

			// convert image
			int map[] = {0};
			int fill[] = {0};
			uint8_pixels = map_channels(uint8_pixels,
				spec->width, spec->height, spec->nchannels,
				channels, map, fill);
			grassmap = interpolate_nearest(uint8_pixels,
				spec->width, spec->height, channels,
				xres, zres);

		} else grassmap = uint8_pixels;

		smf_of.write( (char *)grassmap, xres * zres );
		delete [] grassmap;
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

unsigned short *
uint16_filter_bilinear(unsigned short *in_data,
	int in_w, int in_l, int in_c,
	int out_w, int out_l)
{
	unsigned short *out_data = new unsigned short[out_w * out_l];
	int ix1, ix2, iy1, iy2; //cornering vertices
	float px,py; // sub pixel position on source map.
	float i1, i2, i3; //interpolated values;
	float p1, p2; // pixel colours to interpolate between.

	for( int y = 0; y < out_l; y++) { //iterate through rows
		for( int x = 0; x < out_w; x++) { //iterate through columns
			// sub pixel location on input map.
			px = (float)x / out_w * in_w;
			py = (float)y / out_l * in_l;
			// bounding texels.
			ix1 = floor( px ); ix2 = ceil( px );
			iy1 = floor( py ); iy2 = ceil( py );
			if( ix1 == ix2) ix2++;
			if( iy1 == iy2) iy2++;
			//get the first two points to interpolate between (ix1, iy1) & (ix2, iy1)
			p1 = in_data[ (iy1 * in_w + ix1) * in_c ];
			p2 = in_data[ (iy1 * in_w + ix2) * in_c ];
			// interpolate
			i1 = (p1 * (px - ix1) + p2 * (ix2 - px));
//			printf("%.0f, %.0f, %.0f   ", p1, p2, i1);

			//get the second two points to interpolate between (ix1, iy2) & (ix2, iy2)
			p1 = in_data[ iy2 * in_w + ix1];
			p2 = in_data[ iy2 * in_w + ix2];
			// interpolate
			i2 = (p1 * (px - ix1) + p2 * (ix2 - px));
//			printf("%.0f, %.0f, %.0f   ", p1, p2, i2);

			// Interpolate between those values
			i3 = (i1 * (py - iy1) + i2 * (iy2 - py));
//			printf("%0.0f, %.0f, %.0f\n", i1, i2, i3);

			out_data[y * out_w + x] = (unsigned short)i3;
		}
	}

	delete [] in_data;
	return out_data;
}

unsigned short *
uint16_filter_trilinear(unsigned short *in_data,
	int in_w, int in_l, int in_c,
	int out_w, int out_l)
{
	unsigned short *out_data = new unsigned short[out_w * out_l];
	unsigned short *mip0 = new unsigned short[ in_w/2 * in_l/2];
	int ix1, ix2, iy1, iy2; //cornering vertices
	float px,py; // sub pixel position on source map.
	float i1, i2, i3, i4; //interpolated values;
	float p1, p2; // pixel colours to interpolate between.

	// generate mip level 1 using nearest
	for ( int y = 0; y < in_l /2 ; y++ ) {
		for ( int x = 0; x < in_w /2 ; x++ ) {
			// pixel position
			px = (float)x / 2;
			py = (float)y / 2;
			mip0[ y * in_w / 2 + x ] = in_data[ (int)(py * in_w + px) * in_c];
		}
	}

	// trilinear interpolation
	for( int y = 0; y < out_l; y++) { //iterate through rows
		for( int x = 0; x < out_w; x++) { //iterate through columns
			// sub pixel location on input map.
			px = (float)x / out_w * in_w;
			py = (float)y / out_l * in_l;
			// bounding texels.
			ix1 = floor( px ); ix2 = ceil( px );
			iy1 = floor( py ); iy2 = ceil( py );
			if( ix1 == ix2) ix2++;
			if( iy1 == iy2) iy2++;
			//get the first two points to interpolate between (ix1, iy1) & (ix2, iy1)
			p1 = in_data[ (iy1 * in_w + ix1) * in_c ];
			p2 = in_data[ (iy1 * in_w + ix2) * in_c ];
			// interpolate
			i1 = (p1 * (px - ix1) + p2 * (ix2 - px));
//			printf("%.0f, %.0f, %.0f   ", p1, p2, i1);

			//get the second two points to interpolate between (ix1, iy2) & (ix2, iy2)
			p1 = in_data[ iy2 * in_w + ix1];
			p2 = in_data[ iy2 * in_w + ix2];
			// interpolate
			i2 = (p1 * (px - ix1) + p2 * (ix2 - px));
//			printf("%.0f, %.0f, %.0f   ", p1, p2, i2);

			// Interpolate between those values
			i3 = (i1 * (py - iy1) + i2 * (iy2 - py));
//			printf("%0.0f, %.0f, %.0f\n", i1, i2, i3);

			// get pixel data from mip and interpolate again.
			px = (float)x / out_w * in_w / 2;
			py = (float)y / out_l * in_l / 2;
			i4 = (i3 + mip0[(int)(py * in_w/2 + px)]) / 2;

			out_data[y * out_w + x] = (unsigned short)i4;
		}
	}

	delete [] in_data;
	delete [] mip0;
	return out_data;
}


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
