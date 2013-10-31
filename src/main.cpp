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
#include "tools.h"
#include "smt.h"
#include "smf.h"

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
//  -l --load <string>, smf file to load
//  -s --save <string>, prefix for output
//  -v --verbose, Display extra output
//  -q --quiet, Supress standard output
//  -d --decompile, Decompile loaded map
//  -c --slowcomp, better quality but slower
	string saveFile= "", loadFile = "";
	bool verbose = false, quiet = false, decompile = false, slowcomp = false;
//
//  Map Dimensions
//	-x --width <int>, Width in spring map units
//	-z --length <int>, Length in spring map units
//	-y --floor <float>, Height of the map in spring map units
//	-Y --ceiling <float>, Depth of the map in spring map units
	int width = 2, length = 2;
	float floor = 0, ceiling = 1;
//
// Tiles and Tileindex
//  -a --add-smt <string>
//  -i --tileindex <image>
	vector<string> tileFiles;
	string tileindexFile = "";
//
// Height
//	-e --heightmap <image>
//	   --lowpass, whether to filter the heightmap with a lowpass filter
//	   --invert, whether to invert the heightmap values
	string heightFile = "";
//FIXME	bool lowpass = false, invert = false;
//
// Terrain Type
//	-t --typemap <image>
	string typeFile = "";
//
// Minimap
//  -m --minimap <image>
	string minimapFile = "";
//
// Metal
//	-r --metalmap <image> 
	string metalFile = "";
//
// Grass
//  -g --grassmap <image>
	string grassFile = "";
//
// Features
//	-f --features <text>
	string featuresFile = "";


	try {
		// Define the command line object.
		CmdLine cmd( "Converts a series of image and text files to .smf and"
						".smt files used in SpringRTS maps.", ' ', "0.5" );

		// Define a value argument and add it to the command line.
		
		// Generic Options //		
		/////////////////////
		ValueArg<string> arg_load(
			"l", "load",
			"Filename to load (.smf)",
			false, "", ".smf", cmd);

		ValueArg<string> arg_save(
			"s", "save",
			"Output prefix for the spring map file(.smf)",
			false, "", "fileprefix", cmd);

		SwitchArg arg_verbose(
			"v", "verbose",
			"Extra information printed to standard output.",
			cmd, false );

		SwitchArg arg_quiet(
			"q", "quiet",
			"Supress information printing to standard output.",
			cmd, false );

		SwitchArg arg_decompile(
			"d", "decompile",
			"Supress information printing to standard output.",
			cmd, false );

		SwitchArg arg_slowcomp(
			"c", "slowcomp",
			"Use slower but better compression on DXT1 textures.",
			cmd, false );

		// Map Dimensions //
		////////////////////
		ValueArg<int> arg_width(
			"x", "width",
			"The Width of the map in spring map units. Must be multiples of 2",
			false, 2, "int", cmd );

		ValueArg<int> arg_length(
			"z", "length",
			"The Length of the map in spring map units. Must be multiples of 2",
			false, 2, "int", cmd );

		ValueArg<float> arg_floor(
			"y", "floor",
			"The deepest point on the map in spring map units, with zero being sea level.",
			false, 0, "float", cmd );

		ValueArg<float> arg_ceiling(
			"Y", "ceiling",
			"The highest point on the map in spring map units, with zero being sea level.",
			false, 1, "float", cmd );

		// Tiles and Tileindex //
		/////////////////////////
		MultiArg<string> arg_add_smt(
			"a", "add-smt",
			"External tile files.",
			false, ".smt", cmd );

		ValueArg<string> arg_tileindex(
			"i", "tileindex",
			"The index image to use for referencing tiles.",
			false, "", "image", cmd);

		// Height //
		////////////
		ValueArg<string> arg_height(
			"e", "heightmap",
			"Image to use for vertical displacement of the terrain.",
			false, "", "image", cmd );

		SwitchArg arg_invert(
			"", "invert",
			"Invert the meaning of black and white.",
			cmd, false );

		SwitchArg arg_lowpass(
			"", "lowpass",
			"Lowpass filter smoothing hard edges from 8bit colour.",
			cmd, false );

		// Terrain Type //
		//////////////////
		ValueArg<string> arg_type(
			"t", "typemap",
			"Image to define terrain types.",
			false, "", "image", cmd );

		// Minimap //
		/////////////
		ValueArg<string> arg_minimap(
			"m", "minimap",
			"Image file to use for the minimap",
			false, "", "image", cmd );

		// Metal //
		///////////
		ValueArg<string> arg_metal(
			"r", "metalmap",
			"Image used for built in resourcing scheme.",
			false, "", "image", cmd);

		// Grass //
		///////////
		ValueArg<string> arg_grass(
			"g", "grassmap",
			"Image used to place grass.",
			false, "", "image", cmd );

		// Features //
		//////////////
		ValueArg<string> arg_features(
			"f", "featurelist",
			"Text file defining type, location, rotation of feature and "
			"decal to paint to diffuse texture.",
			false, "", ".csv", cmd );


		// Parse the args.
		cmd.parse( argc, argv );

		// Get the value parsed by each arg.
		// Generic Options
		loadFile = arg_load.getValue();
		saveFile = arg_save.getValue();

		verbose = arg_verbose.getValue();
		quiet = arg_quiet.getValue();
		decompile = arg_decompile.getValue();
		slowcomp = arg_slowcomp.getValue();

		// Map Dimensions
		width = arg_width.getValue();
		length = arg_length.getValue();
		floor = arg_floor.getValue();
		ceiling = arg_ceiling.getValue();

		// SMT options
		tileFiles = arg_add_smt.getValue(); // SMT Input Filenames
		tileindexFile = arg_tileindex.getValue();

		// Height
		heightFile = arg_height.getValue();
//FIXME		invert = arg_invert.getValue();
//FIXME		lowpass = arg_lowpass.getValue();

		// Terrain Type
		typeFile = arg_type.getValue();

		// Minimap
		minimapFile = arg_minimap.getValue();

		// Metal Map
		metalFile = arg_metal.getValue();

		// Grass Map
		grassFile = arg_grass.getValue();

		// Feature List
		featuresFile = arg_features.getValue();


	} catch ( ArgException &e ) {
		cerr << "error: " << e.error() << " for arg " << e.argId() << endl;
		exit( -1 );
	}

	// Test arguments for validity before continuing //
	///////////////////////////////////////////////////
	if ( width < 2 || width > 256 || width % 2 ) {
		if( !quiet )cout << "ERROR: Width needs to be multiples of 2 between and including 2 and 64" << endl;
		fail = true;
	}
	if ( length < 2 || length > 256 || length % 2 ) {
		if( !quiet )cout << "ERROR: Length needs to be multiples of 2 between and including 2 and 64" << endl;
		fail = true;
	}

	if(decompile && !strcmp(loadFile.c_str(), "")) {
		fail = true;
		if( !quiet )cout << "ERROR: no file specified to decompile" << endl;
	}

	if( fail ) exit(-1);

	// Globals //
	/////////////
	SMT smt(verbose, quiet, slowcomp);
	SMF smf(verbose, quiet, slowcomp);

	// Load file 
	if(strcmp(loadFile.c_str(), ""))smf.load(loadFile);

	// decompile loaded file
	if(decompile)
	{
		loadFile.erase(loadFile.size()-4);
		smf.setOutPrefix(loadFile);
		smf.decompileAll(0);
	}

	// Change attributes
	smf.setDimensions(width, length, floor, ceiling);

	if( strcmp(heightFile.c_str(), "") ) smf.setHeightFile( heightFile );
	if( strcmp(typeFile.c_str(), "") ) smf.setTypeFile( typeFile );
	if( strcmp(minimapFile.c_str(), "") ) smf.setMinimapFile( minimapFile );
	if( strcmp(metalFile.c_str(), "") ) smf.setMetalFile( metalFile );
	if( strcmp(tileindexFile.c_str(), "") ) smf.setTileindexFile(tileindexFile );
	if( strcmp(featuresFile.c_str(), "") ) smf.setFeaturesFile( featuresFile );

	if( strcmp(grassFile.c_str(), "") ) smf.setGrassFile(grassFile);
	if( !strcmp(grassFile.c_str(), "") ) smf.unsetGrassFile();

	for(unsigned int i = 0; i < tileFiles.size(); ++i ) {
		smf.addTileFile(tileFiles[i]);
	}
	
	//Save
	if( strcmp(saveFile.c_str(), "") )smf.save( saveFile );
	return 0;
}
