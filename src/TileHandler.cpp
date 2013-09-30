/* c includes */
#include <stdlib.h>
#include <string.h>

/* library includes */
#include <nvtt/nvtt.h>

#include "TileHandler.h"
#include "FileHandler.h"
#include "mapfile.h"

CTileHandler tileHandler;

CTileHandler::CTileHandler()
{
}

CTileHandler::~CTileHandler( void )
{
	int a;
	for ( a = 0; a < usedTiles; ++a ) delete tiles[ a ];
}

void
CTileHandler::LoadTexture( string name )
{
	bigTex.Load( name );

	xsize = bigTex.xsize / 8;
	ysize = bigTex.ysize / 8;
}

// This function generates tile files in a ./temp/ directory.
void
CTileHandler::ProcessTiles( float compressFactor )
{
	meanThreshold = int( 2000 * compressFactor );
	meanDirThreshold = int( 20000 * compressFactor );
	borderThreshold = int( 80000 * compressFactor );
	unsigned char buf[ SMALL_TILE_SIZE ];
	int a,i,j,ox,oy,x2,y2;
	CBitmap *bm;

	vector<string>::iterator fi;

	system( "mkdir temp" );

	usedTiles = 0;

	for ( fi = externalFiles.begin(); fi != externalFiles.end(); ++fi ) {
		ifstream ifs( fi->c_str(), ios::in | ios::binary );

		if ( !ifs.is_open() ) {
			printf( "Couldnt find tile file %s\n", fi->c_str() );
			continue;
		}

		TileFileHeader tfh;
		ifs.read( (char *)&tfh, sizeof( TileFileHeader ) );

		if ( strcmp( tfh.magic, "spring tilefile" ) != 0
			|| tfh.version != 1
			|| tfh.tileSize != 32 ) {
			printf( "Error opening tile file %s\n", fi->c_str() );
			continue;
		}

		externalFileTileSize.push_back( tfh.numTiles );

		printf( "Loading %i tiles from %s\n", tfh.numTiles, fi->c_str() );
		for ( a = 0; a < tfh.numTiles; ++a ) {
			ifs.read( (char *)buf, SMALL_TILE_SIZE );
			bm = new CBitmap();
			bm->CreateFromDXT1( buf, 32, 32 );

			fastStats[ usedTiles ] = CalcFastStat( bm );
			tiles[ usedTiles++ ] = bm;
		}
	}
	numExternalTile = usedTiles;

	// assign memory for new tile image
	unsigned char *data = new unsigned char[ 1024 * 1024 * 4 ];

	int tilex = xsize / 4;
	int tiley = ysize / 4;
	int bigsquaretexx = tilex / 32;
	int bigsquaretexy = tiley / 32;
	int value1,value2;
	a = 0;

	// for each large tile, looping through X and y
	char outfile[1024];
	nvtt::InputOptions inputOptions;
	nvtt::OutputOptions outputOptions;
	nvtt::CompressionOptions compressionOptions;
	nvtt::Compressor compressor;

	inputOptions.setTextureLayout(nvtt::TextureType_2D, 1024, 1024);
//FIXME optional alpha component
//	compressionOptions.setFormat(nvtt::Format_DXT1a);
	compressionOptions.setFormat(nvtt::Format_DXT1);

//FIXME give the user this option.
	compressionOptions.setQuality(nvtt::Quality_Fastest); 
//	compressionOptions.setQuality(nvtt::Quality_Normal); 
	outputOptions.setOutputHeader(false);

	for( j = 0; j < bigsquaretexy; j++ ) {
		for( i = 0; i < bigsquaretexx; i++ ) {
			// Set origin of texture image
			ox = 1024 * i;
			oy = 1024 * j;

			for ( y2 = 0; y2 < 1024; ++y2 ) {
				for ( x2 = 0; x2 < 1024; ++x2 ) {
					// Copy image data
					value1 = (x2 + y2 * 1024) * 4;
					value2 = (ox + (oy + y2) * xsize * 8 + x2) * 4;
					//Copy data to buffer in BGRA format for nvtt to use
					data[ value1 + 2 ] = bigTex.mem[ value2     ];
					data[ value1 + 1 ] = bigTex.mem[ value2 + 1 ];
					data[ value1     ] = bigTex.mem[ value2 + 2 ];
					data[ value1 + 3 ] = bigTex.mem[ value2 + 3 ];
				}
			}

			inputOptions.setMipmapData(data, 1024, 1024);
			sprintf(outfile, "temp/tile%03i", a);
			outputOptions.setFileName( outfile );
			compressor.process(inputOptions, compressionOptions,
						outputOptions);
			printf( "Writing tile %03i of %03i %i%%\n", i + j * bigsquaretexy,
				bigsquaretexy * bigsquaretexx,
				( ( ( a + 1 ) * 1024 ) * 100 ) / ( tilex * tiley ) );
			a++;
		}
	}
	delete[] data;
}

void
CTileHandler::ProcessTiles2( void )
{
	unsigned char *data = new unsigned char[ 1024 * 1024 * 4 ];
	bigTex = CBitmap( data, 1, 1 );	//free big tex memory
	int tilex = xsize / 4;
	int tiley = ysize / 4;
	int bigx = tilex / 32;
	int bigy = tiley / 32;
	int a,b;

	for ( a = 0; a < bigx * bigy; ++a ) {
		int startTilex = (a % bigx) * 32;
		int startTiley = (a / bigx) * 32;
		char name[ 100 ];

		snprintf( name, 100, "temp/tile%03i", a );
		CFileHandler file( name );

		char bigtile[ 696320 ]; // 1024x1024 and 4 mipmaps
		file.Read( bigtile, 696320 );

		for( b = 0; b < 1024; ++b ) {
			int x = b % 32;
			int y = b / 32;
			int xb = startTilex + x; //curr pointer to tile in bigtex
			int yb = startTiley + y;

			char *ctile = new char[ SMALL_TILE_SIZE ];
			ReadTile( x * 32, y * 32, ctile, bigtile );
			CBitmap *bm = new CBitmap();
			bm->CreateFromDXT1( (unsigned char *)ctile, 32, 32 );

			int t1 = tileUse[ max( 0, (yb - 1) * tilex + xb ) ];
			int t2 = tileUse[ max( 0, yb * tilex + xb - 1 ) ];
			int ct = FindCloseTile( bm, t1 == t2 ? t1 : -1 );
			if( ct == -1 ) {
				tileUse[ yb * tilex + xb ] = usedTiles;
				tiles[ usedTiles++ ] = bm;
				newTiles.push_back( ctile );
			} else {
				tileUse[ yb * tilex + xb] = ct;
				delete bm;
				delete[] ctile;
			}
		}
		printf( "Creating tiles %i/%i %i%%\n", usedTiles-numExternalTile,
			(a + 1) * 1024, ( ( (a + 1) * 1024) * 100) / (tilex * tiley) );
	}

	delete[] data;
#ifdef WIN32
	system( "del /q tile*" );
#else
	system( "rm temp/tile*" );
#endif
}

void
CTileHandler::SaveData( ofstream &ofs )
{
	int i,x,y;
	//write tile header
	MapTileHeader mth;
	mth.numTileFiles = (int)externalFiles.size() + 1;
	mth.numTiles = usedTiles;
	ofs.write( (char *)&mth, sizeof( MapTileHeader ) );

	//write external tile files used
	for( i = 0; i < (int)externalFiles.size(); ++i ) {
		ofs.write( (char *)&externalFileTileSize[ i ], 4 );
		ofs.write( externalFiles[ i ].c_str(), 
			externalFiles[ i ].size() + 1 );
	}

	//write new tile file to be created
	int internalTiles = usedTiles - numExternalTile;
	ofs.write( (char *)&internalTiles, 4 );
	ofs.write( myTileFile.c_str(), myTileFile.size() + 1 );

	//write tiles
	for( y = 0; y < ysize / 4; y++ ) {
		for( x = 0; x < xsize / 4; x++ ) {
			i = tileUse[ x + y * xsize / 4 ];
			ofs.write( (char *)&i, sizeof( int ) );
		}
	}

	//create new tile file
	ofstream tf(myTileFile.c_str(),ios::binary | ios::out);

	TileFileHeader tfh;
	strcpy( tfh.magic, "spring tilefile" );
	tfh.version = 1;
	tfh.tileSize = 32;
	tfh.compressionType = 1;
	tfh.numTiles = internalTiles;

	tf.write( (char *)&tfh, sizeof( TileFileHeader ) );

	for( i = 0; i < internalTiles; ++i ) {
		tf.write( newTiles[ i ], SMALL_TILE_SIZE );
		delete[] newTiles[ i ];
	}
}

void
CTileHandler::ReadTile( int xpos, int ypos, char *destbuf, char *sourcebuf )
{
	int doffset = 0;
	int soffset = 0;
	int div,xp,yp;
	int i,j,x,y;
	char *destptr, *srcptr;

	for( i = 0; i < 4; i++)
	{
		div = 1 << i;
		xp = 8 / div;
		yp = 8 / div;
		for ( y = 0; y < yp; y++ ) {
			for( x = 0; x < xp; x++ ) {
				destptr = &destbuf[ (x + y * xp) * 8 + doffset ];
				srcptr = &sourcebuf[
					( (x + xpos / div / 4)
					+ (y + ypos / div / 4) * (256 / div) )
					* 8 + soffset];

				for( j = 0; j < 8; j++ ) destptr[ j ] = srcptr[ j ];
			}
		}
		doffset += 512/(1<<(i*2));
		soffset += 524288/(1<<(i*2));
	}
}

int
CTileHandler::FindCloseTile( CBitmap *bm, int forbidden )
{
	int a;
	FastStat fs = CalcFastStat( bm );
	for( a = 0; a < usedTiles; ++a ) {
		if ( a != forbidden
			&& abs( fs.r - fastStats[ a ].r ) < meanThreshold
			&& abs( fs.g - fastStats[ a ].g ) < meanThreshold
			&& abs( fs.b - fastStats[ a ].b ) < meanThreshold
			&& abs( fs.rx - fastStats[ a ].rx ) < meanDirThreshold
			&& abs( fs.gx - fastStats[ a ].gx ) < meanDirThreshold
			&& abs( fs.bx - fastStats[ a ].bx ) < meanDirThreshold
			&& abs( fs.ry - fastStats[ a ].ry ) < meanDirThreshold
			&& abs( fs.gy - fastStats[ a ].gy ) < meanDirThreshold
			&& abs( fs.by - fastStats[ a ].by ) < meanDirThreshold
			&& CompareTiles( bm, tiles[ a ] ) ) return a;
	}
	fastStats[ usedTiles ] = fs;
	return -1;
}

CTileHandler::FastStat
CTileHandler::CalcFastStat( CBitmap* bm )
{
	int x,y;
	FastStat fs;
	fs.r = 0;
	fs.g = 0;
	fs.b = 0;

	fs.rx = 0;
	fs.gx = 0;
	fs.bx = 0;

	fs.ry = 0;
	fs.gy = 0;
	fs.by = 0;

	for( y = 0; y < 32; ++y ) {
		for( x = 0; x < 32; ++x ) {
			fs.r += bm->mem[ (y * 32 + x) * 4     ];
			fs.g += bm->mem[ (y * 32 + x) * 4 + 1 ];
			fs.b += bm->mem[ (y * 32 + x) * 4 + 2 ];

			fs.rx += bm->mem[ (y * 32 + x) * 4     ] * (x - 16);
			fs.gx += bm->mem[ (y * 32 + x) * 4 + 1 ] * (x - 16);
			fs.bx += bm->mem[ (y * 32 + x) * 4 + 2 ] * (x - 16);

			fs.ry += bm->mem[ (y * 32 + x) * 4     ] * (y - 16);
			fs.gy += bm->mem[ (y * 32 + x) * 4 + 1 ] * (y - 16);
			fs.by += bm->mem[ (y * 32 + x) * 4 + 2 ] * (y - 16);
		}
	}
	return fs;
}

bool
CTileHandler::CompareTiles( CBitmap *bm, CBitmap *bm2 )
{
	int x,y;
	int rdif,bdif,gdif,error,totalerror;
	int value;
	if ( meanThreshold <= 0 )return false;
	for( y = 0; y < 32; ++y ) {
		for( x = 0; x < 32; ++x ) {
			if( !( y == 0 || y == 31 || x == 0 || x == 31) )continue;
			value = (y * 32 + x) * 4;
			rdif = bm->mem[ value     ] - bm2->mem[ value    ];
			gdif = bm->mem[ value + 1 ] - bm2->mem[ value + 1];
			bdif = bm->mem[ value + 2 ] - bm2->mem[ value + 2];
			error = rdif * rdif + gdif * gdif + bdif * bdif;
			totalerror += error;
		}
		if( totalerror > borderThreshold )return false;
	}
	return true;
}

int
CTileHandler::GetFileSize( void )
{
	vector<string>::iterator fi;

	// space needed for tile map
	int size = xsize * ysize / 4; 
	size += sizeof( MapTileHeader );
	for ( fi = externalFiles.begin(); fi != externalFiles.end(); ++fi ) {
		size += 5;	// overhead per file (including 0 termination of string)
		size += (int)fi->size(); //size for filename
	}
	size += (int)myTileFile.size() + 5;	// size and overhead of new tilefile name
	return size;
}

void
CTileHandler::AddExternalTileFile( string file )
{
	externalFiles.push_back( file );
}

void
CTileHandler::SetOutputFile( string file )
{
	myTileFile = file.substr( 0, file.find_last_of( '.' ) ) + ".smt";
}
