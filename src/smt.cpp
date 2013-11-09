#include "smt.h"

#include <fstream>
#include <sys/time.h>
#include <deque>

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include "nvtt_output_handler.h"
#include "smf.h"
#include "dxt1load.h"

TileMip::TileMip()
{
	n = '\0';
}

SMTHeader::SMTHeader()
:	version(1),
	tileRes(32),
	comp(1)
{
	strcpy(magic,"spring tilefile");
}

SMT::SMT()
{
	verbose = true;
	quiet = false;
	slowcomp = false;
	compare = -1;
	stride = 1;
	tileRes = 32;
	nTiles = 0;
	setType(1);
	return;
}

void SMT::setPrefix(string prefix) { outPrefix = prefix.c_str(); }
void SMT::setTileindex(string filename) { tilemapFile = filename.c_str(); }
void SMT::setDim(int w, int l) { width = w; length = l; }

void
SMT::setType(int comp)
{
	this->comp = comp;
	tileSize = 0;
	// Calculate the size of the tile data
	// size is the raw format of dxt1 with 4 mip levels
	// 32x32, 16x16, 8x8, 4x4
	// 512  + 128  + 32 + 8 = 680
	if(comp == DXT1) {
		int mip = tileRes;
		for( int i=0; i < 4; ++i) {
			tileSize += mip/4 * mip/4 * 8;
			mip /= 2;
		}
	} else {
		if( !quiet )printf("ERROR: '%i' is not a valid compression type", comp);
		tileSize = -1;
	}
	
	return;
}

void SMT::addTileSource(string filename) { sourceFiles.push_back(filename); }

bool
SMT::load(string fileName)
{
	loadFile = fileName;
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
		cout << "\tTiles: " << header.nTiles << endl;
		cout << "\tTileRes: " << header.tileRes << endl;
		cout << "\tCompression: ";
		if(header.comp == DXT1)cout << "dxt1" << endl;
		else {
			cout << "UNKNOWN" << endl;
			fail = true;
		}
	}
	setType(header.comp);

	inFile.close();
	return fail;
}

bool
SMT::decompile()
{
	ImageBuf *bigBuf = getBig();

	char filename[256];
	sprintf(filename, "%s_tiles.png", outPrefix.c_str());
	bigBuf->save(filename);
	return false;
}

bool
SMT::save()
{
	// Make sure we have some source images before continuing
	if( sourceFiles.size() == 0) {
		if( !quiet )cout << "ERROR: No source images to convert" << endl;
		return true;
	}

	// Build SMT Header //
	//////////////////////
	char filename[256];
	sprintf(filename, "%s.smt", outPrefix.c_str());

	if( verbose ) printf("\nINFO: Creating %s\n", filename );
	fstream smt( filename, ios::binary | ios::out );
	if( !smt.good() ) {
		cout << "ERROR: fstream error." << endl;
		return true;
	}

	SMTHeader header;
	header.tileRes = tileRes;
	header.comp = comp;
	if( verbose ) {
		printf( "\tVersion: %i.\n", header.version);
		printf( "\tnTiles: tbc.\n");
		printf( "\ttileRes: (%i,%i)%i.\n", tileRes, tileRes, 4);
		printf( "\tcompType: %i.\n", comp);
		printf( "\ttileSize: %i bytes.\n", tileSize);
	}

	smt.write( (char *)&header, sizeof(SMTHeader) );
	smt.close();

	// Build Tileindex & Tiles //
	/////////////////////////////
	
	// setup size for index dimensions
	int tcx = width * 16; // tile count x
	int tcz = length * 16; // tile count z
	unsigned int *indexPixels = new unsigned int[tcx * tcz];

	// Load source image
	ImageBuf *bigBuf = buildBig();
	ImageSpec bigSpec = bigBuf->spec();

	//FIXME process decal baking


	// Swizzle channels
	ImageBuf fixBuf;
	int map[] = {2,1,0,3};
	ImageBufAlgo::channels(fixBuf, *bigBuf, 4, map);
	bigBuf->copy(fixBuf);
	fixBuf.clear();

	vector<TileMip> tileMips;
	TileMip tileMip;

	// Generate and write tiles
	smt.open(filename, ios::binary | ios::out | ios::app );
	ROI roi;
	timeval t1, t2;
    double elapsedTime;
	deque<double> readings;
	double averageTime = 0;
	double intervalTime = 0;
	int totalTiles = tcx * tcz;
	int currentTile;
	// loop through tile columns
	for ( int z = 0; z < tcz; z++) {
		// loop through tile rows
		for ( int x = 0; x < tcx; x++) {
			currentTile = z * tcx + x + 1;
    		gettimeofday(&t1, NULL);

			// copy a tile from the scanline data
			roi.xbegin = x * tileRes;
			roi.xend = x * tileRes + tileRes;
			roi.ybegin = z * tileRes;
			roi.yend = z * tileRes + tileRes;
			roi.zbegin = 0;
			roi.zend = 1;
			roi.chbegin = 0;
			roi.chend = 4;

			ImageBuf tileBuf;
			ImageBufAlgo::crop( tileBuf, *bigBuf, roi );
			ImageSpec tileSpec = tileBuf.spec();
					
			// get handle to raw pixel data for dxt processing
			unsigned char *std = (unsigned char *)tileBuf.localpixels();

			// process into dds
			NVTTOutputHandler *nvttHandler = new NVTTOutputHandler(tileSize);

			nvtt::InputOptions inputOptions;
			inputOptions.setTextureLayout( nvtt::TextureType_2D, tileRes, tileRes );
			inputOptions.setMipmapData( std, tileRes, tileRes );

			nvtt::CompressionOptions compressionOptions;
			compressionOptions.setFormat(nvtt::Format_DXT1a);

			nvtt::OutputOptions outputOptions;
			outputOptions.setOutputHeader(false);
			outputOptions.setOutputHandler( nvttHandler );

			nvtt::Compressor compressor;
			if( slowcomp ) compressionOptions.setQuality(nvtt::Quality_Normal); 
			else           compressionOptions.setQuality(nvtt::Quality_Fastest); 
			compressor.process(inputOptions, compressionOptions, outputOptions);
			
			tileBuf.clear();

			// FIXME perform some better optimisation here
			//copy mipmap to use as checksum: 64bits
			memcpy(&tileMip, nvttHandler->buffer, 8);

			bool match = false;
			unsigned int i; //tile index
			if(compare >=0 ) {
				for( i = 0; i < tileMips.size(); i++ ) {
					if( !strcmp( (char *)&tileMips[i], (char *)&tileMip ) ) {
						match = true;
						break;
					} 
				}
			}
			if( !match ) {
				tileMips.push_back(tileMip);
				// write tile to file.
				smt.write( nvttHandler->buffer, tileSize );
				nTiles +=1;
			}
			delete nvttHandler;
			// Write index to tilemap
			indexPixels[currentTile-1] = i;

    		gettimeofday(&t2, NULL);
			// compute and print the elapsed time in millisec
	    	elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
	    	elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
			readings.push_back(elapsedTime);
			if(readings.size() > 1000)readings.pop_front();
			intervalTime += elapsedTime;
			if( verbose && intervalTime > 1 ) {
				for(unsigned int i = 0; i < readings.size(); ++i)
					averageTime+= readings[i];
				averageTime /= readings.size();
				intervalTime = 0;
			   	printf("\033[0GINFO: Tile %i of %i %%%0.1f complete | %%%0.1f savings | %0.1fs remaining.",
				currentTile, totalTiles,
				(float)currentTile / totalTiles * 100,
				(float)(1 - (float)nTiles / (float)currentTile) * 100,
			    averageTime * (totalTiles - currentTile) / 1000);
			}
		}
	}
	printf("\n");
	smt.close();

	// retroactively fix up the tile count.
	smt.open(filename, ios::binary | ios::in | ios::out );
	smt.seekp( 20);
	smt.write( (char *)&nTiles, 4);
	smt.close();

	// Save tileindex
	ImageOutput *imageOutput;
	sprintf( filename, "%s_tilemap.exr", outPrefix.c_str() );
	
	imageOutput = ImageOutput::create(filename);
	if( !imageOutput ) {
		delete [] indexPixels;
		return true;
	}

	ImageSpec tilemapSpec( tcx, tcz, 1, TypeDesc::UINT);
	imageOutput->open( filename, tilemapSpec );
	imageOutput->write_image( TypeDesc::UINT, indexPixels );
	imageOutput->close();

	delete imageOutput;
	delete [] indexPixels;

	return false;

}

ImageBuf *
SMT::getTile(int tile)
{
	ImageBuf *imageBuf = NULL;
	ImageSpec imageSpec( header.tileRes, header.tileRes, 4, TypeDesc::UINT8 );
	unsigned char *data;

	ifstream smt( loadFile.c_str() );
	if( smt.good() ) {
		unsigned char *temp = new unsigned char[ tileSize ];
		smt.seekg( sizeof(SMTHeader) + tile * tileSize );
		smt.read( (char *)temp, tileSize );
		data = dxt1_load( temp, header.tileRes, header.tileRes );
		delete [] temp;
		char name[11];
		sprintf(name, "tile%6i", tile);
		imageBuf = new ImageBuf( name, imageSpec, data);
	}
	smt.close();
	return imageBuf;	
}

ImageBuf *
SMT::buildBig()
{
	if( verbose ) {
		printf("INFO Pre-processing source images\n");
		printf("    number of source files: %i\n", (int)sourceFiles.size() );
		printf("    stride: %i\n", stride );
		if( sourceFiles.size() % stride != 0 )
			printf("WARNING: number of source files isnt divisible by stride,"
				" black spots will exist\n");
	}

	// Get values to fix
	ImageBuf *regionBuf = new ImageBuf(sourceFiles[0]);
	regionBuf->read(0,0,false, TypeDesc::UINT8);
	if( !regionBuf->initialized() ) {
		if( !quiet )printf("ERROR: could not build big image\n");
		return NULL;
	}
	ImageSpec regionSpec = regionBuf->spec();
	delete regionBuf;

	// Construct the big buffer
	ImageSpec bigSpec(regionSpec.width * stride,
		regionSpec.height * sourceFiles.size() / stride, 4, TypeDesc::UINT8 );
	if( verbose )printf("    Constructed Image: (%i,%i)%i\n", bigSpec.width,
		bigSpec.height, bigSpec.nchannels );
	ImageBuf *bigBuf = new ImageBuf( "big", bigSpec);
	const float fill[] = {0,0,0,255};
	ImageBufAlgo::fill( *bigBuf, fill);

	// Collate the source Files
	for( unsigned int i = 0; i < sourceFiles.size(); ++i ) {
		if( verbose )printf("\033[0GINFO: Copying: %s to big image",
			sourceFiles[i].c_str() );
		regionBuf = new ImageBuf(sourceFiles[i]);
		regionBuf->read(0,0,false, TypeDesc::UINT8);
		if( !regionBuf->initialized() ) {
			if( !quiet ) {
					printf("\nERROR: %s could not be loaded.\n",
						sourceFiles[i].c_str() );
			}
			continue;
		}
		regionSpec = regionBuf->spec();

		int dx = regionSpec.width * (i % stride);
		int dy = regionSpec.height * (i / stride);
		dy = bigSpec.height - dy - regionSpec.height;

		ImageBufAlgo::paste(*bigBuf, dx, dy, 0, 0, *regionBuf);	
	}
	cout << endl;

	// rescale constructed big image to wanted size
	ROI roi(0, width * 512 , 0, length * 512, 0, 1, 0, 4);
	ImageBuf fixBuf;
	if(bigSpec.width != roi.xend || bigSpec.height != roi.yend ) {
		if( verbose )
			printf( "WARNING: Constructed image is (%i,%i), wanted (%i, %i),"
				" Resampling.\n",
			bigSpec.width, bigSpec.height, roi.xend, roi.yend );
		ImageBufAlgo::resample( fixBuf, *bigBuf, true, roi);
		bigBuf->copy(fixBuf);
	}

	return bigBuf;
}

ImageBuf *
SMT::getBig()
{
	ImageBuf *bigBuf = NULL;
	if( !tilemapFile.compare("") ) {
		bigBuf = collateBig();
	} else {
		bigBuf = reconstructBig();
	}
	return bigBuf;
}

ImageBuf *
SMT::collateBig()
{
	if( verbose )cout << "INFO: Collating Big\n";
	if( !loadFile.compare("") ) {
		if( !quiet )cout << "ERROR: No SMT Loaded." << endl;
		return NULL;
	}
	// OpenImageIO
	ImageBuf *tileBuf = NULL;

	// Allocate Image size large enough to accomodate the tiles,
	int collateStride = ceil(sqrt(header.nTiles));
	int bigRes = collateStride * header.tileRes;
	ImageSpec bigSpec(bigRes, bigRes, 4, TypeDesc::UINT8 );
	ImageBuf *bigBuf = new ImageBuf( "big", bigSpec);


	// Loop through tiles copying the data to our image buffer
	for(int i = 0; i < header.nTiles; ++i ) {
		// Pull data
		tileBuf = getTile(i);

		int dx = header.tileRes * (i % collateStride);
		int dy = header.tileRes * (i / collateStride);

		ImageBufAlgo::paste(*bigBuf, dx, dy, 0, 0, *tileBuf);

		delete [] (unsigned char *)tileBuf->localpixels();
		delete tileBuf;
	}

	return bigBuf;
}

ImageBuf *
SMT::reconstructBig()
{
	if( verbose )cout << "INFO: Reconstructing Big\n";
	ImageBuf *tileBuf = NULL;

	//Load tilemap from SMF
	ImageBuf *tilemapBuf = NULL;
	if( is_smf( tilemapFile ) ) {
		SMF sourcesmf(tilemapFile);
		tilemapBuf = sourcesmf.getTilemap();
	}
	// Else load tilemap from image
	if( !tilemapBuf ) {
		tilemapBuf = new ImageBuf( tilemapFile );
		tilemapBuf->read(0,0,false,TypeDesc::UINT);
		if( !tilemapBuf->initialized() ) {
			delete tilemapBuf;
			if( !quiet )printf("ERROR: %s cannot be loaded.\n",
				tilemapFile.c_str());
			return NULL;
		}
	}
	//TODO Else load tilemap from csv

	unsigned int *tilemap = (unsigned int *)tilemapBuf->localpixels();
	int xtiles = tilemapBuf->spec().width;
	int ztiles = tilemapBuf->spec().height;

	// allocate enough data for our large image
	ImageSpec bigSpec( xtiles * tileRes, ztiles * tileRes, 4, TypeDesc::UINT8 );
	ImageBuf *bigBuf = new ImageBuf( "big", bigSpec );

	// Loop through tile index
	for( int z = 0; z < ztiles; ++z ) {
		for( int x = 0; x < xtiles; ++x ) {
			int tilenum = tilemap[z * xtiles + x];
			tileBuf = getTile(tilenum);

			int xbegin = tileRes * x;
			int ybegin = tileRes * z;
			ImageBufAlgo::paste(*bigBuf, xbegin, ybegin, 0, 0, *tileBuf);

			delete [] (unsigned char *)tileBuf->localpixels();
			delete tileBuf;
		}
	}

	delete tilemapBuf;
	if( is_smf( tilemapFile ) ) delete [] tilemap;
	
	return bigBuf;	
}
