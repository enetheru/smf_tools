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
#include "tileconv.h"
#include "nvtt_output_handler.h"
#include "tools.h"

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
	bool fast_dxt1 = false;

	// Other
	SMTHeader header;
	bool fail = false;
	ifstream infile;
	ofstream outfile;

	// Tile Information
	int tile_data_size = 0;
	unsigned char *tile_data;
	unsigned char *tpixels;

	// Tile Index Information
	unsigned int *tileindex;
	int tileindex_w, tileindex_l;

	// OpenImageIO
	ImageInput *imageInput = NULL;
	ImageOutput *imageOutput = NULL;
	ImageSpec *imageSpec = NULL;
	int xres, yres, channels;
	unsigned char *pixels;

	// Nvidia Texture Tools
	nvtt::InputOptions inputOptions;
	nvtt::OutputOptions outputOptions;
	nvtt::CompressionOptions compressionOptions;
	nvtt::Compressor compressor;
	NVTTOutputHandler *outputHandler;

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
		fast_dxt1 = arg_fast_dxt1.getValue();

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
			infile.read(header.magic, 16);
			if( strcmp(header.magic, "spring tilefile") ) {
				if ( !quiet )printf( "ERROR: %s is not a valid smt\n", dfn.c_str() );
				fail = true;
			}
		}
		infile.close();
	} //dnf.compare("")

	if( fail )exit ( -1 );

// Decompile //
///////////////
	if( dfn.compare("") ) {
		infile.open(dfn.c_str(), ifstream::in);
		infile.read( (char *)&header, sizeof(SMTHeader));
		if( verbose ) {
			cout << "INFO: " << dfn << endl;
			cout << "\tSMT Version: " << header.version << endl;
			cout << "\tTiles: " << header.tiles << endl;
			cout << "\tTile Size: " << header.size << endl;
			cout << "\tCompression: ";
			if(header.comp == 1)cout << "dxt1" << endl;
			else {
				cout << "UNKNOWN" << endl;
				fail = true;
			}
		}
		if( fail ) {
			cout << "ERROR: Cannot decompile unknown format" << endl;
			exit(-1);
		}
		infile.close();

		// Calculate the size of the tile data
		// size is the raw format of dxt1 with 4 mip levels
		// 32x32, 16x16, 8x8, 4x4
		// 512  + 128  + 32 + 8 = 680
		if(header.comp == 1) {
			int tile_size = header.size;
			for( int i=1; i <= 4; i++) {
				tile_data_size += tile_size/4 * tile_size/4 * 8;
				tile_size /= 2;
			}
		}
		printf("Tile Data Size: %i\n", tile_data_size);

		// Process tiles based on tile index //
		///////////////////////////////////////
		if( tifn.compare("") ) {
			imageInput = NULL;
			imageInput = ImageInput::create( tifn.c_str() );
			if( !imageInput ) {
				cout << "ERROR: reading tile index" << endl;
				exit(-1);
			} else {
				if( verbose ) printf( "INFO: Reading %s.\n", tifn.c_str() );
				imageSpec = new ImageSpec;
				imageInput->open( tifn.c_str(), *imageSpec );
				tileindex_w = imageSpec->width;
				tileindex_l = imageSpec->height;
				tileindex = new unsigned int[ imageSpec->width * imageSpec->height * imageSpec->nchannels ];
				imageInput->read_image( TypeDesc::UINT, tileindex );
				imageInput->close();
				delete imageInput;
				delete imageSpec;
			}
			// allocate enough data for our large image
			pixels = new unsigned char[tileindex_w * 32 * tileindex_l * 32 * 4];

			// Loop through tile index
			infile.open( dfn.c_str(), ifstream::in );
			for( int ty = 0; ty < tileindex_l; ++ty ) {
				for( int tx = 0; tx < tileindex_w; ++tx ) {
					// allocate data for the tile
					tile_data = new unsigned char[tile_data_size];

					// get the index value	
					int tilenum = tileindex[ty * tileindex_w + tx];

					// Pull data from the tile file
					infile.seekg( sizeof(SMTHeader) + tilenum * tile_data_size );
					infile.read( (char *)tile_data, tile_data_size);
					if ( header.comp == 1 ) {
						tpixels = dxt1_load(tile_data, header.size, header.size);
						delete [] tile_data;
					}

					// Copy to our large image
					// TODO save scanlines to large image once a row is filled to save memory
					for ( int y = 0; y < header.size; y++ ) {
						for ( int x = 0; x < header.size; x++ ) {
							// get pixel locations.
							int tp = (y * header.size + x) * 4;
							int dx = header.size * tx + x;
							int dy = header.size * ty + y;
							int dp = (dy * tileindex_w * 32 + dx) * 4;

							memcpy( (char *)&pixels[dp], (char *)&tpixels[tp], 4 );
						}
					}
					delete [] tpixels;
				}
			}
			infile.close();
			delete [] tileindex;

			char filename[256];
			sprintf(filename, "%s_tiles.jpg", ofn.c_str());

			// Save the large image to disk
			imageOutput = NULL;
			imageOutput = ImageOutput::create(filename);
			if(!imageOutput) exit(-1);
			imageSpec = new ImageSpec(tileindex_w * 32, tileindex_l * 32, 4, TypeDesc::UINT8);
			imageOutput->open( filename, *imageSpec );
			imageOutput->write_image( TypeDesc::UINT8, pixels);
			imageOutput->close();
			delete imageOutput;
			delete imageSpec;
			delete [] pixels;

		} else {
			// Process Individual Tiles //
			//////////////////////////////
			// Allocate Image size large enough to accomodate the tiles,
			int tileindex_w = tileindex_l = ceil(sqrt(header.tiles));
			int xres = yres = tileindex_w * header.size;
			channels = 4;
			pixels = new unsigned char[xres * yres * channels];

			char filename[256];
			sprintf(filename, "%s_tiles.jpg", ofn.c_str());

			imageOutput = NULL;
			imageOutput = ImageOutput::create(filename);
			if(!imageOutput) exit(-1);
			imageSpec = new ImageSpec(xres, yres, channels, TypeDesc::UINT8);

			// Loop through tiles copying the data to our image buffer
			//TODO save scanlines to file once a row is complete to
			//save on memory
			infile.open(dfn.c_str(), ifstream::in);
			infile.seekg( sizeof(SMTHeader) );
			for(int i = 0; i < header.tiles; ++i ) {
				// Pull data
				tile_data = new unsigned char[tile_data_size];
				infile.read( (char *)tile_data, tile_data_size);
				if ( header.comp == 1 ) {
					tpixels = dxt1_load(tile_data, header.size, header.size);
				}
				delete [] tile_data;

				for ( int y = 0; y < header.size; y++ ) {
					for ( int x = 0; x < header.size; x++ ) {
						// get pixel locations.
						int tp = (y * header.size + x) * 4;
						int dx = header.size * (i % tileindex_w) + x;
						int dy = header.size * (i / tileindex_w) + y;
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
		}

		//TODO
		// Save each tile to an individual file
//		for(int i = 0; i < header.tiles; ++i ) {
			// Pull data
//			rawdds = new unsigned char[tile_data_size];
//			infile.read( (char *)rawdds, tile_data_size);
//			tpixels = dxt1_load(rawdds, header.size, header.size);
//			delete [] rawdds;

//			char filename[256];
//			sprintf(filename, "%s_%010i.jpg", ofn.c_str(), i);

//			imageOutput = NULL;
//			imageOutput = ImageOutput::create(filename);
//			if(!imageOutput) exit(-1);

//			imageSpec = new ImageSpec(header.size, header.size, 4, TypeDesc::UINT8);
//			imageOutput->open( filename, *imageSpec );
//			imageOutput->write_image( TypeDesc::UINT8, pixels);
//			imageOutput->close();
//			delete imageOutput;
//			delete imageSpec;
//			delete [] tpixels;
//		}
		


	} //endif dnf.compare("")


	exit(0);
}
