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
// -e --extract, extract tiles from loaded smt
//    --slow_dxt1, compress slowly but more accurately
	bool verbose = false, quiet = false, extract = false;
	bool slow_dxt1 = false;
//    --cnum <int>, number of tiles to compare to -1 = no comparison, 0 = hashtable comparison, > 0 = numeric comparison.
//    --cpet <float>, pixel error threshold 0.0f-1.0f
//    --cnet <int>, number of errors threshold.
	int compare_num = 32;
	float compare_pixelThresh = 1.0f/255.0f;
	int compare_numThresh = 4;

// -i --input <string>, smt to load
// -o --output <string>, prefix for output
// -t --tilemap <string>, 
	string inputFile = "";
	string outPrefix = "";
	string tilemapFile = "";

// -a --add <string> ..., Source files to use when building tiles;
// -s --stride <int>, number of images horizontally
	vector<string> sourceFiles;
	int stride = 1;

// Map Dimensions
// -w --width <int>
// -l --length <int>
	int width = 2, length = 2;

// Decal Pasting
// -d --decals <.csv>, file describing the images to paste and where to paste
//		them. in the format "filename,x,z" x and z describe the centre of the image.
	string decalFile = "";


// TCLAP Command Line Arguments //
//////////////////////////////////
	try {
		// Define the command line object.
		CmdLine cmd( "Converts image(s) into a SpringRTS map tile (.smt) file.",
			' ', "0.5" );

		// Add options	
		SwitchArg arg_verbose(
			"v", "verbose",
			"Print more information to stdout.",
			cmd, false);

		SwitchArg arg_quiet(
			"q", "quiet",
			"Supress error messages from being sent to stdout.",
			cmd, false);

		SwitchArg arg_extract(
			"e", "extract",
			"decompile loaded SMT.",
			cmd, false);

		SwitchArg arg_slow_dxt1(
			"", "slow_dxt1",
			"Use the higher quality, slower algorithm when generating tiles.",
			cmd, false);

		ValueArg<int> arg_cnum(
			"", "cnum",
			"comparison pixel error threshold. 0.0f-1.0f",
			false, 1.0f/255.0f, "float", cmd);

		ValueArg<float> arg_cpet(
			"", "cpet",
			"comparison pixel error threshold. 0.0f-1.0f",
			false, 1.0f/255.0f, "float", cmd);

		ValueArg<int> arg_cnet(
			"", "cnet",
			"comparison number of errors threshold 0-1024. -1=no comparison.",
			false, 16, "int", cmd);

		ValueArg<string> arg_input(
			"i", "input",
			"load an smt file.",
			false, "", "filename", cmd);

		ValueArg<string> arg_output(
			"o", "output",
			"Prefix to use when saving files.",
			false, "", "string", cmd);

		ValueArg<string> arg_tilemap(
			"t", "tilemap",
			"The tilemap to use when reconstructing the diffuse texture.",
			false, "", "filename", cmd);

		ValueArg<int> arg_width(
			"w", "width",
			"Width of the map used when constructing the tile index.",
			false, 2, "int", cmd);

		ValueArg<int> arg_length(
			"l", "length",
			"length of the map used when constructing the tile index.",
			false, 2, "int", cmd);

		ValueArg<int> arg_stride(
			"s", "stride",
			"number of image horizontally.",
			false, 1, "int", cmd);

		ValueArg<string> arg_decals(
			"d", "decals",
			"file to parse when pasting decals.",
			false, "", "*.csv", cmd);

		UnlabeledMultiArg<string> arg_addSource(
			"a",
			"source files to use for tiles",
			false,
			"images", cmd);

		// Parse Arguments
		cmd.parse( argc, argv );

		// Get values
		verbose = arg_verbose.getValue();
		quiet = arg_quiet.getValue();
		slow_dxt1 = arg_slow_dxt1.getValue();
		extract = arg_extract.getValue();

		inputFile = arg_input.getValue();
		outPrefix = arg_output.getValue();
		tilemapFile = arg_tilemap.getValue();

		stride = arg_stride.getValue();
		sourceFiles = arg_addSource.getValue();
		decalFile = arg_decals.getValue();

		length = arg_length.getValue();
		width = arg_width.getValue();

		compare_num = arg_cnum.getValue();
		compare_pixelThresh = arg_cpet.getValue();
		compare_numThresh = arg_cnet.getValue();

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
	smt.slow_dxt1 = slow_dxt1;
	smt.cnum = compare_num;
	smt.cpet = compare_pixelThresh;
	smt.cnet = compare_numThresh;
	smt.stride = stride;

	smt.setSize( width, length );

	smt.setDecalFile( decalFile );
	smt.setTilemap( tilemapFile );

	if( !inputFile.empty() ) {
		smt.load( inputFile );
		inputFile.erase( inputFile.size() - 4 );
		smt.setPrefix( inputFile );
		if( extract ) smt.decompile();
	} else {
		vector< string >::iterator it;
		for(it = sourceFiles.begin(); it != sourceFiles.end(); it++ )
			smt.addTileSource( *it );
	}

	if( !outPrefix.empty() ) {
		smt.setPrefix( outPrefix );
		smt.save();
	}

	exit(0);
}
