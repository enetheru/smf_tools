#include "smt.h"

#include <fstream>

#include "tclap/CmdLine.h"
using namespace TCLAP;

#ifdef WIN32
int
_tmain( int argc, _TCHAR *argv[] )
#else
int
main( int argc, char **argv )
#endif
{
	bool fail = false;
	ifstream infile;

	// OpenImageIO
	ImageInput *imageInput = NULL;

// Options
// -v --verbose, display extra output
// -q --quiet, supress standard output
// -c --slowcomp, compress slowly but more accurately
// -d --decompile, decompile loaded smt
	bool verbose = false, quiet = false, slowcomp = false, decompile = false;

// -l --load <string>, smt to load
// -s --save <string>, prefix for output
// -i --tileindex <string>, 
	string loadFile = "";
	string saveFile = "";
	string tileindexFile = "";

// -a --add-image <string>, Source files to use when building tiles;
	vector<string> imageFiles;

// Map Dimensions
// -x --width <int>
// -z --length <int>
	int width = 2, length = 2;


// TCLAP Command Line Arguments //
//////////////////////////////////
	try {
		// Define the command line object.
		CmdLine cmd( "Converts image(s) into a SpringRTS map tile (.smt) file.", ' ', "0.5" );

		// Add options	
		SwitchArg arg_verbose(
			"v", "verbose",
			"Print more information to stdout.",
			cmd, false);

		SwitchArg arg_quiet(
			"q", "quiet",
			"Supress error messages from being sent to stdout.",
			cmd, false);

		SwitchArg arg_slowcomp(
			"c", "slowcomp",
			"Use the higher quality, slower algorithm when generating tiles.",
			cmd, false);

		SwitchArg arg_decompile(
			"d", "decompile",
			"decompile loaded SMT.",
			cmd, false);

		ValueArg<string> arg_load(
			"l", "load",
			"load an smt file.",
			false, "", "filename", cmd);

		ValueArg<string> arg_save(
			"s", "save",
			"Prefix to use when saving files.",
			false, "", "string", cmd);

		ValueArg<string> arg_tileindex(
			"i", "tileindex",
			"Filename of the tile index to use when reconstructing the diffuse texture.",
			false, "", "filename", cmd);

		MultiArg<string> arg_add_source(
			"a", "add-source",
			"files.",
			false, ".smt", cmd );

		ValueArg<int> arg_width(
			"x", "width",
			"Width of the map used when constructing the tile index.",
			false, 2, "int", cmd);

		ValueArg<int> arg_length(
			"z", "length",
			"length of the map used when constructing the tile index.",
			false, 2, "int", cmd);

		// Parse Arguments
		cmd.parse( argc, argv );

		// Get values
		verbose = arg_verbose.getValue();
		quiet = arg_quiet.getValue();
		slowcomp = arg_slowcomp.getValue();
		decompile = arg_decompile.getValue();

		loadFile = arg_load.getValue();
		saveFile = arg_save.getValue();
		tileindexFile = arg_tileindex.getValue();
		imageFiles = arg_add_source.getValue();

		length = arg_length.getValue();
		width = arg_width.getValue();

	} catch ( ArgException &e ) {
		cerr << "error: " << e.error() << " for arg " << e.argId() << endl;
		exit( -1 );
	}

	SMT smt(verbose, quiet, slowcomp);

// Test Arguments //
////////////////////
	if ( width < 2 || width > 256 || width % 2 ) {
		cout << "ERROR: Width needs to be multiples of 2 between and including 2 and 64" << endl;
		fail = true;
	}
	if ( length < 2 || length > 256 || length % 2 ) {
		cout << "ERROR: Length needs to be multiples of 2 between and including 2 and 64" << endl;
		fail = true;
	}

	// Test TileIndex //
	if( tileindexFile.compare( "" ) ) {
		imageInput = NULL;
		imageInput = ImageInput::create( tileindexFile.c_str() );
		if ( !imageInput ) {
			if ( !quiet )cout << "ERROR: tileindex image missing or invalid format." << endl;
			fail = true;
		}
		delete imageInput;
	}

	if( fail )exit ( -1 );

// Decompile //
///////////////

	if( loadFile.compare("") ) {
		printf( "TEST: smt.load(%s)\n", loadFile.c_str());
		smt.load(loadFile);
	}

	if(decompile) {
		loadFile.erase(loadFile.size()-4);
		smt.setPrefix(loadFile);
		if( tileindexFile.compare("") ) {
			printf( "TEST: smt.setTileindex(%s)\n", tileindexFile.c_str());
			smt.setTileindex(tileindexFile);
			printf( "TEST: smt.decompileReconstruct()\n");
			smt.decompileReconstruct();
		} else {
			printf( "TEST: smt.decompileCollate()\n");
			smt.decompileCollate();
		}
	}

	for(unsigned int i = 0; i < imageFiles.size(); ++i ) {
		printf( "TEST: smt.addImage(%s)\n", imageFiles[i].c_str());
		smt.addImage(imageFiles[i]);
	}

	smt.setDim(width, length);

	if( saveFile.compare("") ) {
		printf( "TEST: smt.save()\n");
		smt.setPrefix(saveFile);
		smt.save();
	}

	exit(0);
}
