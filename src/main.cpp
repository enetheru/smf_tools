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

unsigned char *
uint8_image_convert(unsigned char *id, //input data
	int iw, int il, int ic, // input width, length, channels
	int ow, int ol, int oc, // output width, length, channels
	int *map, int *fill); // channel map, fill map

//static void
//HeightMapInvert(int mapx, int mapy);

//static void
//HeightMapFilter(int mapx, int mapy);

#ifdef WIN32
int
_tmain( int argc, _TCHAR *argv[] )
#else
int
main( int argc, char **argv )
#endif
{
// Generic Options
//  -o --smf-ofn <string>
//     --smt-ofn <string>
//     --no-smt <bool> Don't build smt file
//     --smt <smt file> Use existing smt file
//  -v --verbose <bool> Display extra output
//  -q --quiet <bool> Supress standard output
	string smf_ofn = "", smt_ofn = "";
	string smt_ifn = "";
//	bool no_smt = true,
	bool verbose = false, quiet = false;
//
//  Map Dimensions
//	-w --width <int> Width in spring map units
//	-l --length <int> Length in spring map units
//	-a --height <float> Height of the map in spring map units
//	-d --depth <float> Depth of the map in spring map units
	int width = 0, length = 0, height = 1, depth = 1;
//
// Diffuse Image
//	-c --diffuse <image> 
//	   --fast-dxt1 <bool> fast compression for dxt1 of diffuse image
	bool fast_dxt1 = false;
	string diffuse_fn = "";
//
// Displacement
//	-y --displacement <image>
//	   --lowpass <bool>
//	   --invert <bool>
	string displacement_fn = "";
	bool displacement_lowpass = false, displacement_invert = false;
//
// Metal
//	-m --metal <image>
	string metalmap_fn = "";
//
// Terrain Type
//	-t --type <image>
	string typemap_fn = "";
//
// Features
//	-f --feature <image>
//	   --featurelist <text>
	string featuremap_fn = "", featurelist_fn = "";
//
// Minimap
//     --minimap <image>
	string minimap_fn = "";

	// Variables I use a lot!! //
	///////////////////////////////////
	ImageInput *in; 
	ImageSpec *spec;
	int xres, zres, channels;
	int x, z, ox, oz;
	unsigned short *uint16_pixels;
	unsigned char *uint8_pixels;

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
			"o", "smf-ofn",
			"Output prefix for the spring map file(.smf)",
			true, "", "string", cmd);

		ValueArg<string> arg_smt_ofn(
			"", "smt-ofn",
			"Output prefix for the spring map tile file(.smt), will default "
			" to the spring map file name",
			false, "", "string", cmd);

		ValueArg<string> arg_smt_ifn(
			"", "smt-ifn",
			"External tile file that will be used for finding tiles. Tiles"
			"not found in this will be saved in a new tile file.",
			false, "", "filename", cmd );

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
		ValueArg<string> arg_featuremap_fn(
			"f", "featuremap",
			"Image used to place features and grass.",
			false, "", "filename", cmd );

		ValueArg<string> arg_featurelist_fn(
			"", "featurelist",
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
		smf_ofn = arg_smf_ofn.getValue();// SMF Output Filename
		smt_ofn = arg_smt_ofn.getValue();// SMT Output Filename
		smt_ifn = arg_smt_ifn.getValue();// SMT Input Filename
		quiet = arg_quiet.getValue();
		verbose = arg_verbose.getValue();

		// Map Dimensions
		width = arg_width.getValue();
		length = arg_length.getValue();
		depth = arg_depth.getValue();
		height = arg_height.getValue();

		// Diffuse Image
		diffuse_fn = arg_diffuse_fn.getValue();
		fast_dxt1 = arg_fast_dxt1.getValue();

		// Displacement
		displacement_fn = arg_displacement_fn.getValue();
		displacement_invert = arg_displacement_invert.getValue();
		displacement_lowpass = arg_displacement_lowpass.getValue();

		// Resources
		metalmap_fn = arg_metalmap_fn.getValue();

		// Terrain Type
		typemap_fn = arg_typemap_fn.getValue();

		// Features
		featuremap_fn = arg_featuremap_fn.getValue();
		featurelist_fn = arg_featurelist_fn.getValue();

		// Minimap
		minimap_fn = arg_minimap_fn.getValue();

	} catch ( ArgException &e ) {
		cerr << "error: " << e.error() << " for arg " << e.argId() << endl;
		exit( -1 );
	}

	// Test arguments for validity before continuing //
	///////////////////////////////////////////////////
	cout << "INFO: Testing Arguments for validity." << endl;
	if ( width < 2 || width > 256 || width % 2 ) {
		cout << "ERROR: Width needs to be multiples of 2 between and including 2 and 64" << endl;
		exit( -1 );
	}
	if ( length < 2 || length > 256 || length % 2 ) {
		cout << "ERROR: Length needs to be multiples of 2 between and including 2 and 64" << endl;
		exit( -1 );
	}

	// Test SMT name
	if( !smt_ofn.compare( "" ) ) smt_ofn = smf_ofn;

	// Test Diffuse //
	if( !diffuse_fn.compare( "" ) ) {
		if ( verbose ) cout << "WARNING: Diffuse unspecified.\n\tMap will be grey." << endl;
	} else {
		in = ImageInput::create( diffuse_fn.c_str() );
		if ( !in ) {
			if ( !quiet )cout << "ERROR: Diffuse image missing or invalid format." << endl; 
			exit( -1 );
		}
		delete in;
	}
	// Test Displacement //
	if( !displacement_fn.compare( "" ) ) {
		if ( verbose ) cout << "WARNING: Displacement unspecified.\n\tMap will be flat." << endl;
	} else {
		in = ImageInput::create( displacement_fn.c_str() );
		if ( !in ) {
			if ( !quiet )cout << "ERROR: Displacement image missing or invalid format." << endl;
			exit( -1 );
		}
		delete in;
	}
	// Test Metalmap //
	if( !metalmap_fn.compare( "" ) ) {
		if ( verbose ) cout << "WARNING: Metalmap unspecified." << endl;
	} else {
		in = ImageInput::create( metalmap_fn.c_str() );
		if ( !in ) {
			if ( !quiet )cout << "ERROR: Metalmap image missing or invalid format." << endl;
			exit( -1 );
		}
		delete in;
	}
	// Test Typemap //
	if( !typemap_fn.compare( "" ) ) {
		if ( verbose ) cout << "WARNING: Typemap unspecified." << endl;
	} else {
		in = ImageInput::create( typemap_fn.c_str() );
		if ( !in ) {
			if ( !quiet )cout << "ERROR: Typemap image missing or invalid format." << endl;
			exit( -1 );
		}
		delete in;
	}
	// Test Featuremap //
	if( !featuremap_fn.compare( "" ) ) {
		if ( verbose ) cout << "WARNING: Featuremap unspecified." << endl;
	} else {
		in = ImageInput::create( featuremap_fn.c_str() );
		if ( !in ) {
			if ( !quiet )cout << "ERROR: Featuremap image missing or invalid format." << endl;
			exit( -1 );
		}
		delete in;
	}
	// Test Minimap //
	if( !minimap_fn.compare( "" ) ) {
		if ( verbose ) cout << "WARNING: Minimap unspecified." << endl;
	} else {
		in = ImageInput::create( minimap_fn.c_str() );
		if ( !in ) {
			if ( !quiet )cout << "ERROR: Minimap image missing or invalid format." << endl;
			exit( -1 );
		}
		delete in;
	}

	// Setup Global variables //
	////////////////////////////
	compressionOptions.setFormat(nvtt::Format_DXT1a);
	if( fast_dxt1 ) compressionOptions.setQuality(nvtt::Quality_Fastest); 
	else compressionOptions.setQuality(nvtt::Quality_Normal); 
	outputOptions.setOutputHeader(false);
	

	// SMT file comes before SMF because SMF relies on the tiles and filename
	// Build SMT Header //
	//////////////////////
	if( verbose ) printf("INFO: Creating %s.smt\n", smt_ofn.c_str() );
	ofstream smt_of( (smt_ofn+".smt").c_str(), ios::binary | ios::out);
	TileFileHeader tileFileHeader;

	strcpy( tileFileHeader.magic, "spring tilefile" );
	tileFileHeader.version = 1;
	tileFileHeader.tileSize = 32; 
	tileFileHeader.compressionType = 1; 
	tileFileHeader.numTiles = 0; //FIXME re-write this later after we know the number of tiles
	smt_of.write( (char *)&tileFileHeader, sizeof(TileFileHeader) );

	xres = width * 512;
	zres = length * 512;
	channels = 4;

	int tcx = width * 16; // tile count x
	int tcz = length * 16; // tile count z

	int stw, stl, stc = 4; // source tile dimensions
	int dtw, dtl, dtc = 4; //destination tile dimensions
//	int ttw, ttl, ttc = 4; // temporary tile info.
//	ttw = ttl = tileFileHeader.tileSize;
	dtw = dtl = tileFileHeader.tileSize;

	unsigned char *std, *dtd; // source & destination tile pixel data
/*	unsigned char *ttd; // temporary tile pixel data

	// used of no diffuse image is created.
	ttd = new unsigned char[ttw * ttl * ttc];
	for (int z = 0; z < ttl; ++z ) // rows
		for ( int x = 0; x < ttw; ++x ) { // columns
			ttd[ (z * ttw + x) * ttc + 0 ] = 39; //red
			ttd[ (z * ttw + x) * ttc + 1 ] = 123; //green
			ttd[ (z * ttw + x) * ttc + 2 ] = 81; //blue
			ttd[ (z * ttw + x) * ttc + 3 ] = 255; //alpha
	} */

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
			printf( "INFO: Loaded %s (%i, %i)*%i\n",
				diffuse_fn.c_str(), spec->width, spec->height, spec->nchannels);
			printf( "INFO: Converting tile size (%i,%i)*%i to (%i,%i)*%i\n",
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
				dtd = uint8_image_convert(std,
							stw, stl, stc,
							dtw, dtl, dtc,
							map, fill);

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
	if( verbose ) cout << "INFO: Creating Header." << endl;
#ifdef WIN32
	LARGE_INTEGER li;
	QueryPerformanceCounter( &li );
#endif

	MapHeader header;
	strcpy( header.magic, "spring map file" );
	header.version = 1;

#ifdef WIN32
	// FIXME: this should be made better to make it depend on heightmap etc,
	// but this should be enough to make each map unique
	header.mapid = li.LowPart; 
#else
	header.mapid = time( NULL );
#endif
	header.mapx = width * 64;
	header.mapy = length * 64;
	header.squareSize = 8;
	header.texelPerSquare = 8;
	header.tilesize = 32;
	header.minHeight = depth * 512;
	header.maxHeight = height * 512;

	header.numExtraHeaders = 1;

	int offset = sizeof( MapHeader );
	// add size of vegetation extra header
	offset += 12;
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
	offset += (int)(smf_ofn + ".smt").size() + 5; // new tilefile name
	offset += width * length  * 1024;  // space needed for tile map
	//FIXME requires changes to allow multiple smt's
	header.metalmapPtr = offset;

	// add size of metalmap
	offset += width * length * 1024;

	// add size of vegetation map
	offset += width * length * 256;	
	header.featurePtr = offset;

	// Write Header to file //
	//////////////////////////
	if( verbose ) cout << "INFO: Writing Header." << endl;
	ofstream smf_of( (smf_ofn + ".smf").c_str(), ios::out | ios::binary );
	smf_of.write( (char *)&header, sizeof( MapHeader ) );

	//extra header size
	int temp = 12;	
	smf_of.write( (char *)&temp, 4 );

	//extra header type
	temp = MEH_Vegetation;	
	smf_of.write( (char *)&temp, 4 );

	//offset to vegetation map
	temp = header.metalmapPtr + width * length * 1024;
	smf_of.write( (char *)&temp, 4 );

	// Height Displacement //
	/////////////////////////
	if( verbose ) cout << "INFO: Processing and writing displacement." << endl;
	xres = width * 64 + 1;
	zres = length * 64 + 1;
	channels = 1;
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

		// resample nearest
		displacement = new unsigned short[ xres * zres ];
		for (int z = 0; z < zres; z++ ) // rows
			for ( int x = 0; x < xres; x++ ) { // columns
				ox = (float)x / xres * spec->width;
				oz = (float)z / zres * spec->height;
				displacement[ z * xres + x ] =
					uint16_pixels[ (oz * spec->width + ox) * spec->nchannels ];
		}
		delete [] uint16_pixels;
	} else displacement = uint16_pixels;

	// If inversion of height is specified in the arguments,
	if ( displacement_invert && verbose ) cout << "WARNING: Height inversion not implemented yet" << endl;

	// If inversion of height is specified in the arguments,
	if ( displacement_lowpass && verbose ) cout << "WARNING: Height lowpass filter not implemented yet" << endl;
	
	// write height data to smf.
	smf_of.write( (char *)displacement, xres * zres * 2 );
	delete [] displacement;
	delete spec;

	// Typemap //
	/////////////
	if( verbose ) cout << "INFO: Processing and writing typemap." << endl;
	xres = width * 32;
   	zres = length * 32;
	channels = 1;
	unsigned char *typemap;

	in = ImageInput::create( typemap_fn.c_str() );
	if( !in ) {
			spec = new ImageSpec(xres, zres, 1, TypeDesc::UINT8);
			uint8_pixels = new unsigned char[xres * zres];
			memset( uint8_pixels, 0, xres * zres);

	} else {
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
		typemap = uint8_image_convert(uint8_pixels,
			spec->width, spec->height, spec->nchannels,
			xres, zres, channels,
			map, fill);
		
	} else typemap = uint8_pixels;

	smf_of.write( (char *)typemap, xres * zres );
	delete [] typemap;
	delete spec;

	// Minimap //
	/////////////
	if( verbose ) cout << "INFO: Processing and writing minimap." << endl;
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
		minimap = uint8_image_convert(uint8_pixels,
			spec->width, spec->height, spec->nchannels,
			xres, zres, channels,
			map, fill);

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
	///////////////
	MapTileHeader mapTileHeader;
	mapTileHeader.numTileFiles = 1; //FIXME needs changing to allow multiple smt's
	mapTileHeader.numTiles = width * length * 256;
	smf_of.write( (char *)&mapTileHeader, sizeof( MapTileHeader ) );


	smf_of.write( (char *)&mapTileHeader.numTiles, 4 );
	smf_of.write( (smf_ofn + ".smt").c_str(), smf_ofn.size() + 5 );

	// Write tile index
	int i;
	for ( int z = 0; z < length * 16; z++ )
		for ( int x = 0; x < width * 16; x++) {
			i = x + z * width * 16;
			smf_of.write( (char *)&i, sizeof( int ) );
	}

	// Metal Map //
	///////////////
	if( verbose ) cout << "INFO: Processing and writing metalmap." << endl;
	xres = width * 32;
	zres = length * 32;
	channels = 1;
	unsigned char *metalmap;

	in = ImageInput::create( metalmap_fn.c_str() );
	if( !in ) {
		if( verbose ) cout << "WARNING: Creating blank metalmap." << endl;
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
		metalmap = uint8_image_convert(uint8_pixels,
			spec->width, spec->height, spec->nchannels,
			xres, zres, channels,
			map, fill);

	} else metalmap = uint8_pixels;

	smf_of.write( (char *)metalmap, xres * zres );

//	featureCreator.WriteToFile( &smf_of, F_Spec );

	smf_of.close();
	return 0;
}

/*
static void
HeightMapInvert(int mapx, int mapy)
{
	int x,y;
	float *heightmap2;

	printf( "Inverting height map\n" );
	heightmap2 = heightmap;
	heightmap = new float[ mapx * mapy ];
	for ( y = 0; y < mapy; ++y )
		for( x = 0; x < mapx; ++x )
			heightmap[ y * mapx +x ] =	heightmap2[ (mapy -y -1) * mapx +x ];

	delete[] heightmap2;
}

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

unsigned char *
uint8_image_convert(unsigned char *id, //input data
	int iw, int il, int ic, // input width, length, channels
	int ow, int ol, int oc, // output width, length, channels
	int *map, int *fill) // channel map, fill map
{
	int ix, iy; // origin coordinates
	int ip, op; // offsets to location in memory
	if(ow == -1) ow = iw; 
	if(ol == -1) ol = il;
	if(oc == -1) oc = ic;

	unsigned char *od = new unsigned char[ow * ol * oc];

	for (int y = 0; y < ol; ++y ) // output rows
		for ( int x = 0; x < ow; ++x ) { // output columns
			ix = (float)x / ow * iw; //calculate the input coordinates
			iy = (float)y / ol * il;

			op = (y * ow + x) * oc; // calculate output data location
			ip = (iy * iw + ix) * ic; // calculate input data location

			for ( int c = 0; c < oc; ++c ) // output channels
				if( map[ c ] >= ic ) od[ op +c ] = fill[ c ];
				else od[ op + c ] = id[ ip + map[ c ] ];
	}

	return od;
}
