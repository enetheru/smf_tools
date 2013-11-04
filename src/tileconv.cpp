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

// Options
// -v --verbose, display extra output
// -q --quiet, supress standard output
//    --slowcomp, compress slowly but more accurately
// -d --decompile, decompile loaded smt
// -c --compare, compare tiles when saving
	bool verbose = false, quiet = false, decompile = false;
	int compare = -1;
	bool slowcomp = false;

// -l --load <string>, smt to load
// -s --save <string>, prefix for output
// -i --tileindex <string>, 
	string loadFile = "";
	string saveFile = "";
	string tileindexFile = "";

// -a --add-image <string>, Source files to use when building tiles;
// -? --stride <int>, number of images horizontally
	vector<string> imageFiles;
	int stride = 1;

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
			"", "slowcomp",
			"Use the higher quality, slower algorithm when generating tiles.",
			cmd, false);

		ValueArg<int> arg_compare(
			"c", "compare",
			"-1 is none, 0 is exact matches, > 0 is variance allowed.",
			false, -1, "int", cmd);

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

		ValueArg<int> arg_width(
			"x", "width",
			"Width of the map used when constructing the tile index.",
			false, 2, "int", cmd);

		ValueArg<int> arg_length(
			"z", "length",
			"length of the map used when constructing the tile index.",
			false, 2, "int", cmd);

		ValueArg<int> arg_stride(
			"", "stride",
			"number of image horizontally.",
			false, 1, "int", cmd);

		UnlabeledMultiArg<string> arg_add_images(
			"a",
			"description",
			false,
			"images", cmd);

		// Parse Arguments
		cmd.parse( argc, argv );

		// Get values
		verbose = arg_verbose.getValue();
		quiet = arg_quiet.getValue();
		slowcomp = arg_slowcomp.getValue();
		compare = arg_compare.getValue();
		decompile = arg_decompile.getValue();

		loadFile = arg_load.getValue();
		saveFile = arg_save.getValue();
		tileindexFile = arg_tileindex.getValue();
		stride = arg_stride.getValue();
		imageFiles = arg_add_images.getValue();

		length = arg_length.getValue();
		width = arg_width.getValue();

	} catch ( ArgException &e ) {
		cerr << "error: " << e.error() << " for arg " << e.argId() << endl;
		exit( -1 );
	}

// Test Arguments //
////////////////////
//FIXME

	if( fail ) {
		return true;
	}
	
	SMT smt;
	smt.verbose = verbose;
	smt.quiet = quiet;
	smt.slowcomp = slowcomp;
	smt.compare = compare;
	smt.stride = stride;

	if( loadFile.compare("") ) {
		smt.load(loadFile);
	}

	if(decompile) {
		loadFile.erase(loadFile.size()-4);
		smt.setPrefix(loadFile);
		if( tileindexFile.compare("") ) {
			smt.setTileindex(tileindexFile);
			smt.decompileReconstruct();
		} else {
			smt.decompileCollate();
		}
	}

	for(unsigned int i = 0; i < imageFiles.size(); ++i ) {
		smt.addImage(imageFiles[i]);
	}

	smt.setDim(width, length);

	if( saveFile.compare("") ) {
		smt.setPrefix(saveFile);
		smt.save();
	}

	exit(0);
}
