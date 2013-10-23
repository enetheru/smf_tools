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

// Local
//#include "tileconv.h"
#include "nvtt_output_handler.h"
#include "tools.h"
#include "smt.h"

using namespace std;
using namespace TCLAP;
OIIO_NAMESPACE_USING

#ifdef WIN32
int
_tmain( int argc, _TCHAR *argv[] )
#else
int
main( int argc, char **argv )
#endif
{
// Global Variables //
//////////////////////

	SMT smt;
	char magic[16];
	bool fail = false;
	ifstream infile;

	// OpenImageIO
	ImageInput *imageInput = NULL;

	// Options
// -o --output-filename
	string ofn = "out";
// -i --input-filename
	string ifn = "";
// -d --decompile-filename
	string dfn = "";
// -t --tileindex-filename
	string tifn = "";
// -v --verbose
	bool verbose = false;
// -q --quiet
	bool quiet = false;
// -l --length
	int length = 2;
// -w --width
	int width = 2;
// -f --fast-dxt1
//	bool fast_dxt1 = false;


// TCLAP Command Line Arguments //
//////////////////////////////////
	try {
		// Define the command line object.
		CmdLine cmd( "Converts image(s) into a SpringRTS map tile (.smt) file.", ' ', "0.5" );

		// Add options	
		ValueArg<string> arg_ofn(
			"o", "output-filename",
			"Output prefix used when saving the spring tile file and tile index.",
			false, "smt_dump", "string", cmd);

		ValueArg<string> arg_ifn(
			"i", "input-filename",
			"Image file, or prefix for image files used for constructing the tile file.",
			false, "", "string", cmd);

		ValueArg<string> arg_dfn(
			"d", "decompile-filename",
			"Filename of spring tile file to decompile.",
			false, "", "string", cmd);

		ValueArg<string> arg_tifn(
			"t", "tileindex-filename",
			"Filename of the tile index to use when reconstructing the diffuse texture.",
			false, "", "string", cmd);

		SwitchArg arg_verbose(
			"v", "verbose",
			"Print more information to stdout.",
			cmd, false);

		SwitchArg arg_quiet(
			"q", "quiet",
			"Supress error messages from being sent to stdout.",
			cmd, false);

		SwitchArg arg_fast_dxt1(
			"f", "fast-dxt1",
			"Use the lower quality, faster algorithm when generating tiles.",
			cmd, false);

		ValueArg<int> arg_width(
			"w", "width",
			"Width of the map used when constructing the tile index.",
			false, 2, "int", cmd);

		ValueArg<int> arg_length(
			"l", "length",
			"length of the map used when constructing the tile index.",
			false, 2, "int", cmd);


		// Parse Arguments
		cmd.parse( argc, argv );

		// Get values
		ofn = arg_ofn.getValue();
		ifn = arg_ifn.getValue();
		dfn = arg_dfn.getValue();
		tifn = arg_tifn.getValue();

		verbose = arg_verbose.getValue();
		quiet = arg_quiet.getValue();
		length = arg_length.getValue();
		width = arg_width.getValue();

		smt.verbose = verbose;
		smt.quiet = quiet;
		//fast_dxt1 = arg_fast_dxt1.getValue();

	} catch ( ArgException &e ) {
		cerr << "error: " << e.error() << " for arg " << e.argId() << endl;
		exit( -1 );
	}

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
	if( tifn.compare( "" ) ) {
		imageInput = NULL;
		imageInput = ImageInput::create( tifn.c_str() );
		if ( !imageInput ) {
			if ( !quiet )cout << "ERROR: Grassmap image missing or invalid format." << endl;
			fail = true;
		}
		delete imageInput;
	}

	// Test decompile file exists
	if( dfn.compare("") ) {
		infile.open(dfn.c_str(), ifstream::in);
		if(!infile.good()) {
			if ( !quiet )printf( "ERROR: Cannot open '%s'\n", dfn.c_str() ); 
			fail = true;
		} else {
			infile.read(magic, 16);
			if( strcmp(magic, "spring tilefile") ) {
				if ( !quiet )printf( "ERROR: %s is not a valid smt\n", dfn.c_str() );
				fail = true;
			}
		}
		infile.close();
	} //dnf.compare("")

	if( fail )exit ( -1 );

// Decompile //
///////////////

	if( !dfn.compare("") ) {
		smt.load(dfn);
		if( tifn.compare("") ) {
			smt.loadTileIndex(tifn);
			smt.decompileReconstruct(ofn);
		} else smt.decompileCollate(ofn);
	}


	exit(0);
}
