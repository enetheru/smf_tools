// MapConv.cpp : Defines the entry point for the console application.
//#define WIN32 true

#include <string>
#include <stdio.h>
#include <fstream>
#include <vector>

// c headers
#include <math.h>

// external
#include "tclap/CmdLine.h"
#ifdef WIN32
#include "ddraw.h"
#include "stdafx.h"
#else
#include "time.h" /* time() */
#endif

// local headers
#include "mapfile.h"
#include "FileHandler.h"
#include "FeatureCreator.h"
#include "TileHandler.h"
#include "Bitmap.h"

using namespace std;
using namespace TCLAP;

void
ConvertTextures( string intexname, string temptexname, int xsize, int ysize );

void
LoadHeightMap( string inname, int xsize, int ysize, float minHeight,
				float maxHeight,bool invert,bool lowpass );

void
SaveHeightMap( ofstream &outfile, int xsize, int ysize, float minHeight,
				float maxHeight );

void
SaveTexOffsets( ofstream &outfile, string temptexname, int xsize, int ysize );

void
SaveTextures( ofstream &outfile, string temptexname, int xsize, int ysize );

void
SaveMiniMap( ofstream &outfile );

void
SaveMetalMap( ofstream &outfile, std::string metalmap, int xsize, int ysize );

void
SaveTypeMap( ofstream &outfile, int xsize, int ysize, string typemap );

void
MapFeatures( const char *ffile, char *F_Array );

CFeatureCreator featureCreator;
float *heightmap;
short int *rotations;
int randomrotatefeatures = 0;

#ifndef WIN32
string stupidGlobalCompressorName;
#else
string stupidGlobalCompressorName = "nvdxt.exe -nmips 4 -dxt1a -Sinc -file";
#endif

#ifdef WIN32
int
_tmain( int argc, _TCHAR *argv[] )
#else
int
main( int argc, char **argv )
#endif
{
	printf( "Mothers Mapconv version 2.4 updated by Beherith"
				   "(mysterme at gmail dot com)\n" );
	int xsize;
	int ysize;
	string intexname = "mars.bmp";
	string inHeightName = "mars.raw";
	string outfilename = "mars.smf";
	string metalmap = "marsmetal.bmp";
	string typemap = "";
	string extTileFile = "";
	string featuremap = "";
	string geoVentFile = "geovent.bmp";
	string featureListFile = "fs.txt";
	string featurePlaceFile = "fp.txt";
	float minHeight = 20;
	float maxHeight = 300;
	float compressFactor = 0.8f;
//	float whereisit = 0;
	bool invertHeightMap = false;
	bool lowpassFilter = false;
	bool usenvcompress = false;
//	bool justsmf = false;
	vector<string> F_Spec;

	// exmple command
	// -i -c 0.7 -x 608 -n -76 -o Schizo_Shores_v4.smf -m m2.bmp -t t2.bmp
	// -a h3.raw -f f2.bmp -z "nvdxt2.exe -dxt1a -Box -quality_production
	// -nmips 4 -fadeamount 0 -sharpenMethod SharpenSoft -file"

	try {
		// Define the command line object.
		CmdLine cmd( "Converts a series of image files to a Spring map. This"
			"just creates the .smf and .smt files. You also need to write a"
			".smd file using a text editor.", ' ', "0.5" );

		// Define a value argument and add it to the command line.
		ValueArg<string> intexArg(
			"t",
			"intex",
			"Input bitmap to use for the map. Sides must be multiple of 1024"
			"long. xsize, ysize determined from this file: xsize = intex"
			"width / 8, ysize = height / 8.",
			true,
			"test.bmp",
			"texturemap file" );
		cmd.add( intexArg );

		ValueArg<string> heightArg(
			"a",
			"heightmap",
			"Input heightmap to use for the map, this should be in 16 bit"
			"raw format (.raw extension) or an image file. Must be xsize*64+1"
			"by ysize*64+1.",
			true,
			"test.raw",
			"heightmap file" );
		cmd.add( heightArg );

		ValueArg<string> metalArg(
			"m",
			"metalmap",
			"Metal map to use, red channel is amount of metal. Resized to"
			"xsize / 2 by ysize / 2.",
			true,
			"metal.bmp",
			"metalmap image ");
		cmd.add( metalArg );

		ValueArg<string> typeArg(
			"y",
			"typemap",
			"Type map to use, uses the red channel to decide type. types are"
			"defined in the .smd, if this argument is skipped the map will be"
			"all type 0",
			false,
			"",
			"typemap image" );
		cmd.add( typeArg );

		ValueArg<string> geoArg(
			"g",
			"geoventfile",
			"The decal for geothermal vents; appears on the compiled map at"
			"each vent. (Default: geovent.bmp).",
			false,
			"geovent.bmp",
			"Geovent image" );
		cmd.add( geoArg );

		ValueArg<string> tileArg(
			"e",
			"externaltilefile",
			"External tile file that will be used for finding tiles. Tiles"
			"not found in this will be saved in a new tile file.",
			false,
			"",
			"tile file" );
		cmd.add( tileArg );

		ValueArg<string> outArg(
			"o",
			"outfile",
			"The name of the created map file. Should end in .smf. A"
			"tilefile (extension .smt) is also created.",
			false,
			"test.smf",
			"output .smf" );
		cmd.add( outArg );

		ValueArg<float> minhArg(
			"n",
			"minheight",
			"What altitude in spring the min(0) level of the height map"
			"represents.",
			true,
			-20,
			"min height" );
		cmd.add( minhArg );

		ValueArg<float> maxhArg(
			"x",
			"maxheight",
			"What altitude in spring the max(0xff or 0xffff) level of the"
			"height map represents.",
			true,
			500,
			"max height" );
		cmd.add( maxhArg );

		ValueArg<float> compressArg(
			"c",
			"compress",
			"How much we should try to compress the texture map. Default 0.8,"
			"lower -> higher quality, larger files.",
			false,
			0.8f,
			"compression" );
		cmd.add( compressArg );

		#ifdef WIN32
		char *defaultTexCompress = "nvdxt.exe -nmips 4 -dxt1a -Sinc -file";
		stupidGlobalCompressorName = "nvdxt.exe -nmips 4 -dxt1a -Sinc -file";
		#else
		const char *defaultTexCompress = "texcompress_nogui";
		#endif

		ValueArg<string> texCompressArg(
			"z",
			"texcompress",
			"Name of companion program texcompress from current working"
			"directory.",
			false,
			defaultTexCompress,
			"texcompress program" );
		cmd.add( texCompressArg );

		// Actually, it flips the heightmap *after* it's been read in.
		// Hopefully this is clearer.
		SwitchArg invertSwitch(
			"i",
			"invert",
			"Flip the height map image upside-down on reading.",
			false );
		cmd.add( invertSwitch );

		SwitchArg usenvcompressSwitch(
			"q",
			"use_nvcompress",
			"Use NVCOMPRESS.EXE tool for ultra fast CUDA compression. Needs"
			"Geforce 8 or higher nvidia card.",
			false );
		cmd.add( usenvcompressSwitch );

		SwitchArg lowpassSwitch(
			"l",
			"lowpass",
			"Lowpass filters the heightmap",
			false );
		cmd.add( lowpassSwitch );
		
		SwitchArg justsmfSwitch(
			"s",
			"justsmf",
			"Just create smf file, dont make smt",
			false );
		cmd.add( justsmfSwitch );
		
		ValueArg<int> rrArg(
			"r",
			"randomrotate",
			"rotate features randomly, the first r features in featurelist"
			"(fs.txt) get random rotation, default 0",
			false,
			0,
			"randomrotate" );
		cmd.add( rrArg );

		ValueArg<string> featureArg(
			"f",
			"featuremap",
			"Feature placement file, xsize by ysize. See README.txt for"
			"details. Green 255 pixels are geo vents, blue is grass, green"
			"201-215 is default trees, red 255-0 correspont each to a line"
			"in fs.txt.",
			false,
			"",
			"featuremap image" );
		cmd.add( featureArg );

		ValueArg<string> featureListArg(
			"j",
			"featurelist",
			"A file with the name of one feature on each line. (Default:"
			"fs.txt). See README.txt for details. \n Specifying a number"
			"from 32767 to -32768 next to the feature name will tell mapconv"
			"how much to rotate the feature. specifying -1 will rotate it"
			"randomly. \n Example line from fs.txt \n btreeblo_1 -1\n"
			"btreeblo 16000",
			false,
			"fs.txt",
			"feature list file" );
		cmd.add( featureListArg );

		ValueArg<string> featurePlaceArg(
			"k",
			"featureplacement",
			"A special text file defining the placement of each feature."
			"(Default: fp.txt). See README.txt for details. The default"
			"format specifies it to have each line look like this (spacing"
			"is strict) \n { name = \'agorm_talltree6\', x = 224, z = 3616,"
			"rot = \"0\" }",
			false,
			"fp.txt",
			"feature placement file" );
		cmd.add( featurePlaceArg );

		// Parse the args.
		cmd.parse( argc, argv );

		// Get the value parsed by each arg.
		intexname = intexArg.getValue();
		inHeightName = heightArg.getValue();
		outfilename = outArg.getValue();
		typemap = typeArg.getValue();
		extTileFile = tileArg.getValue();
		metalmap = metalArg.getValue();
		minHeight = minhArg.getValue();
		maxHeight = maxhArg.getValue();
		compressFactor = compressArg.getValue();
		invertHeightMap = invertSwitch.getValue();
		lowpassFilter = lowpassSwitch.getValue();
		featuremap = featureArg.getValue();
		usenvcompress = usenvcompressSwitch.getValue();
		geoVentFile = geoArg.getValue();
		featureListFile = featureListArg.getValue();
		stupidGlobalCompressorName = texCompressArg.getValue();
//		justsmf = justsmfSwitch.getValue();
		randomrotatefeatures = rrArg.getValue();
		featurePlaceFile = featurePlaceArg.getValue();
	} catch ( ArgException &e ) {
		cerr << "error: " << e.error() << " for arg " << e.argId() << endl;
		exit( -1 );
	}

	tileHandler.LoadTexture( intexname );
	tileHandler.SetOutputFile( outfilename );
	if ( !extTileFile.empty() )tileHandler.AddExternalTileFile( extTileFile );

	xsize = tileHandler.xsize;
	ysize = tileHandler.ysize;

	LoadHeightMap( inHeightName, xsize, ysize, minHeight, maxHeight,
				invertHeightMap, lowpassFilter );

	ifstream ifs;
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
		numNamedFeatures, featuremap, geoVentFile, extrafeatures, F_Spec);

	printf( "Options are: -q: %i compressstring: %s \n", (int)usenvcompress,
		stupidGlobalCompressorName.c_str() );

	if ( usenvcompress && stupidGlobalCompressorName.find( "nvdxt" ) > 0 ) {
		stupidGlobalCompressorName = "nvcompress.exe -fast -bc1";
		printf( "Invalid compressor string for CUDA compress, using default"
			"of nvcompress.exe -fast -bc1\n" );
	}

	tileHandler.ProcessTiles( compressFactor, usenvcompress );

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
	header.mapx = xsize;
	header.mapy = ysize;
	header.squareSize = 8;
	header.texelPerSquare = 8;
	header.tilesize = 32;
	header.minHeight = minHeight;
	header.maxHeight = maxHeight;

	header.numExtraHeaders = 1;

	int headerSize = sizeof( MapHeader );
	headerSize += 12; //size of vegetation extra header

	header.heightmapPtr = headerSize;
	header.typeMapPtr = header.heightmapPtr + (xsize +1) * (ysize +1) * 2;
	header.minimapPtr = header.typeMapPtr + (xsize / 2) * (ysize / 2);
	header.tilesPtr = header.minimapPtr + MINIMAP_SIZE;
	header.metalmapPtr = header.tilesPtr + tileHandler.GetFileSize();
	header.featurePtr = header.metalmapPtr
		+ (xsize / 2) * (ysize / 2)
		+ (xsize / 4 * ysize / 4);	//last one is space for vegetation map


	ofstream outfile( outfilename.c_str(), ios::out | ios::binary );

	outfile.write( (char *)&header, sizeof( MapHeader ) );

	//extra header size
	int temp = 12;	
	outfile.write( (char *)&temp, 4 );

	//extra header type
	temp = MEH_Vegetation;	
	outfile.write( (char *)&temp, 4 );

	//offset to vegetation map
	temp = header.metalmapPtr + (xsize / 2) * (ysize / 2);
	outfile.write( (char *)&temp, 4 );

	SaveHeightMap( outfile, xsize, ysize, minHeight, maxHeight );

	SaveTypeMap( outfile, xsize, ysize, typemap );
	SaveMiniMap( outfile );

	tileHandler.ProcessTiles2();
	tileHandler.SaveData( outfile );

	SaveMetalMap( outfile, metalmap, xsize, ysize );

	featureCreator.WriteToFile( &outfile, F_Spec );

	delete[] heightmap;
	return 0;
}

void
SaveMiniMap( ofstream &outfile )
{
	printf( "creating minimap\n" );

	CBitmap mini = tileHandler.bigTex.CreateRescaled( 1024, 1024 );
	mini.Save( "mini.bmp" );
#ifdef WIN32
	try system( "nvdxt.exe -dxt1a -file mini.bmp\n" );
	// we are looking for a stack overflow except, running this program in
	// debug mode throws an exception at this point...
	catch(...) printf( "caught a wierd stack overflow exception (this doesnt"
		"mean the program failed...)\n" );

	DDSURFACEDESC2 ddsheader;
	int ddssignature;

	CFileHandler file( "mini.dds" );
	file.Read( &ddssignature, sizeof( int ) );
	file.Read( &ddsheader, sizeof( DDSURFACEDESC2 ) );
#else
	char execstring[ 512 ];
	snprintf( execstring, 512, "%s mini.bmp",
					stupidGlobalCompressorName.c_str() );
	system( execstring );
	CFileHandler file( "mini.bmp.raw" );
#endif

	char minidata[ MINIMAP_SIZE ];
	file.Read( minidata, MINIMAP_SIZE );
	outfile.write( minidata, MINIMAP_SIZE );
}

void
LoadHeightMap( string inname, int xsize, int ysize, float minHeight,
			float maxHeight, bool invert, bool lowpass )
{
	printf( "Creating height map\n" );

	float hDif = maxHeight - minHeight;
	int mapx = xsize + 1;
	int mapy = ysize + 1;
	heightmap = new float[ mapx * mapy ];

	if ( inname.find( ".raw" ) != string::npos ) { //16 bit raw
		CFileHandler fh(inname);

		for ( int y = 0; y < mapy; ++y )
			for ( int x = 0; x < mapx; ++x ) {
				unsigned short h;
				fh.Read( &h, 2 );
				heightmap[ (ysize-y) * mapx + x ] =
						(float( h )) / 65535 * hDif + minHeight;
			}
	} else {		//standard image
		CBitmap bm( inname );
		if( bm.xsize != mapx || bm.ysize != mapy ) {
			printf( "Errenous dimensions for heightmap image. Correct size"
				"is texture/8 +1\n  You specified %i x %i when %i x %i is"
				"the correct dimension based on the texture\n",
				bm.xsize, bm.ysize, mapx, mapy );
			exit(0);
		}
		for( int y = 0; y < mapy; ++y )
			for( int x = 0; x < mapx; ++x ) {
				unsigned short h = (int)bm.mem[ (y * mapx + x) * 4 ] * 256;
				heightmap[ (ysize - y) * mapx + x ] =
						(float( h )) / 65535 * hDif+minHeight;
			}
	}

	if ( invert ) {
		printf( "Inverting height map\n" );
		float *heightmap2 = heightmap;
		heightmap = new float[ mapx * mapy ];
		for ( int y = 0; y < mapy; ++y )
			for( int x = 0; x < mapx; ++x ) {
				heightmap[ y * mapx + x ] =
					heightmap2[ (mapy - y - 1) * mapx + x ];
			}
		delete[] heightmap2;
	}

	if ( lowpass ) {
		printf( "Applying lowpass filter to height map\n" );
		float *heightmap2 = heightmap;
		heightmap = new float[ mapx * mapy ];
		for ( int y = 0; y < mapy; ++y ) {
			for ( int x = 0; x < mapx; ++x ) {
				float h = 0;
				float tmod = 0;
				for ( int y2 = max( 0, y - 2 );
						y2 < min( mapy, y + 3 );
						++y2 ) {
					int dy = y2 - y;
					for ( int x2 = max( 0, x - 2 );
									x2 < min( mapx, x + 3 );
									++x2 ) {
						int dx = x2 - x;
						float mod = max( 0.0f, 1.0f - 0.4f
									* sqrtf( float( dx * dx + dy * dy ) ) );
						tmod += mod;
						h += heightmap2[ y2 * mapx + x2 ] * mod;
					}
				}
				heightmap[ y * mapx + x ] = h / tmod;
			}
		}
		delete[] heightmap2;
	}
}

void
SaveHeightMap( ofstream& outfile, int xsize, int ysize, float minHeight,
			float maxHeight )
{
	int x,y;
	int mapx = xsize + 1;
	int mapy = ysize + 1;
	unsigned short *hm = new unsigned short[ mapx * mapy ];
	float sub = minHeight;
	float mul = ( 1.0f / (maxHeight - minHeight) ) * 0xffff;
	for( y = 0; y < mapy; ++y ) 
		for( x = 0; x < mapx; ++x )
			hm[ y * mapx + x ] =
				(unsigned short)( ( heightmap[ y * mapx + x ] - sub ) * mul );

	outfile.write((char*)hm,mapx*mapy*2);

	delete[] hm;
}

void
SaveMetalMap( ofstream &outfile, std::string metalmap, int xsize, int ysize )
{
	int x,y,size;
	char *buf;

	printf( "Saving metal map\n" );

	CBitmap metal( metalmap );
	if( metal.xsize != xsize / 2 || metal.ysize != ysize / 2 ) {
		metal = metal.CreateRescaled( xsize / 2, ysize / 2 );
		printf( "Warning: Metal map is being rescaled, may result in"
			"undesirable metal layout. Correct size is %i * %i \n",
			xsize / 2, ysize / 2 );
	}

	size = (xsize / 2) * (ysize / 2);
	buf = new char[ size ];

	for(y = 0; y < metal.ysize; ++y )
		for(x = 0; x < metal.xsize; ++x )
			//we use the red component of the picture
			buf[ y * metal.xsize + x ] =
				metal.mem[ (y * metal.xsize + x) * 4 ];	

	outfile.write( buf, size );

	delete [] buf;
}

void
SaveTypeMap( ofstream &outfile, int xsize, int ysize, string typemap )
{
	int a;
	int mapx = xsize / 2;
	int mapy = ysize / 2;

	unsigned char *typeMapMem = new unsigned char[ mapx * mapy ];
	memset( typeMapMem, 0, mapx * mapy );

	if ( !typemap.empty() ) {
		CBitmap tm;
		tm.Load( typemap );
		if( tm.xsize != mapx || tm.ysize != mapy )
			printf("WARNING: TYPEMAP NOT CORRECT SIZE! WILL RESULT IN"
				"RESIZED TYPEMAP WITH HOLES IN IT! Correct size is %i*%i \n",
				mapx, mapy );
		CBitmap tm2 = tm.CreateRescaled( mapx, mapy );
		for ( a = 0; a < mapx * mapy; ++a )
			typeMapMem[ a ] = tm2.mem[ a * 4 ];
	}
	outfile.write( (char *)typeMapMem, mapx * mapy );

	delete[] typeMapMem;
}

