// Bitmap.cpp: implementation of the CBitmap class.
#include <ostream>
#include <fstream>
#include <iostream>
// c Headers
#include <assert.h>
#include <string.h>
// Library Headers
#include <IL/il.h>
// Local headers
#include "filehandler.h"
#include "bitmap.h"
#ifdef WIN32
#include "stdafx.h"
#endif

struct InitializeOpenIL
{
	InitializeOpenIL() {
		ilInit();
	}

	~InitializeOpenIL() {
		ilShutDown();
	}
} static initOpenIL;

CBitmap::CBitmap()
	: xsize( 1 ), ysize( 1 )
{
	mem = new unsigned char[ 4 ];
}

CBitmap::~CBitmap()
{
	//delete[] mem;
}

// Copy Constructor
CBitmap::CBitmap( const CBitmap &old)
{
	xsize = old.xsize;
	ysize = old.ysize;
	mem = new unsigned char[ xsize * ysize * 4 ];
	memcpy( mem, old.mem, xsize * ysize * 4 );
}

// copy raw memory
CBitmap::CBitmap( unsigned char *data, int xsize, int ysize )
	: xsize( xsize ), ysize( ysize )
{
	mem = new unsigned char[ xsize * ysize * 4 ];	
	memcpy( mem, data, xsize * ysize * 4 );
}

// Load image from file
CBitmap::CBitmap( string const &filename )
	: mem( 0 ), xsize( 0 ), ysize( 0 )
{
	Load(filename);
}

// copy operator
CBitmap &CBitmap::operator=( const CBitmap &bm )
{
	if( this != &bm ) {
		delete[] mem;
		xsize = bm.xsize;
		ysize = bm.ysize;
		mem = new unsigned char[ xsize * ysize * 4 ];
		memcpy( mem, bm.mem, xsize * ysize * 4 );
	}
	return *this;
}

void
CBitmap::Load( string const &filename, unsigned char defaultAlpha )
{
	int x,y;
	unsigned char *buffer;
	bool success, noAlpha;
	ILuint ImageName;

	ilOriginFunc(IL_ORIGIN_UPPER_LEFT);
	ilEnable(IL_ORIGIN_SET);

	if ( mem != NULL ) delete [] mem;
	try {
		CFileHandler file( filename );
		if ( file.FileExists() == false) throw 1;
	   	
		buffer = new unsigned char[ file.FileSize() ];
		file.Read( buffer, file.FileSize() );

		ImageName = 0;
		ilGenImages( 1, &ImageName );
		ilBindImage( ImageName );

		success = ilLoadL( IL_TYPE_UNKNOWN, buffer, file.FileSize() );
		delete [] buffer;

		if ( success == false ) throw 2;

		noAlpha = ilGetInteger( IL_IMAGE_BYTES_PER_PIXEL ) != 4;

		ilConvertImage( IL_RGBA, IL_UNSIGNED_BYTE );

		xsize = ilGetInteger( IL_IMAGE_WIDTH );
		ysize = ilGetInteger( IL_IMAGE_HEIGHT );

		mem = new unsigned char[ xsize * ysize * 4 ];
		memcpy( mem, ilGetData(), xsize * ysize * 4 );

		if ( noAlpha ) {
			for ( y = 0; y < ysize; ++y ) {
				for ( x = 0;x < xsize; ++x )
					mem[ ( y * xsize + x ) * 4 + 3 ] = defaultAlpha;
			}
		}
	}
	catch( int e ) {
		if (e == 1)std::cout << "File does not exist: ";
		if (e == 2)std::cout << "Could not load: ";
		std::cout << filename << "\n";
		xsize = 1;
		ysize = 1;
		mem = new unsigned char[ 4 ];
		memset( mem, 0, 4 );
		return;
	}
}


void
CBitmap::Save( string const &filename )
{
	int x,y,v1,v2;
	ilOriginFunc( IL_ORIGIN_UPPER_LEFT );
	ilEnable( IL_ORIGIN_SET );

	unsigned char *buf = new unsigned char[ xsize * ysize * 4 ];
	/* HACK Flip the image so it saves the right way up.
		(Fiddling with ilOriginFunc didn't do anything?)
		Duplicated with ReverseYAxis. */
	for( y = 0; y < ysize; ++y ) {
		for( x = 0; x < xsize; ++x ) {
			v1 = ( (ysize - 1 - y) * xsize + x) * 4;
			v2 = (y * xsize + x) * 4;
			buf[ v1     ] = mem[ v2     ];
			buf[ v1 + 1 ] = mem[ v2 + 1 ];
			buf[ v1 + 2 ] = mem[ v2 + 2 ];
			buf[ v1 + 3 ] = mem[ v2 + 3 ];
		}
	}

	ilHint( IL_COMPRESSION_HINT, IL_USE_COMPRESSION );
	ilSetInteger( IL_JPG_QUALITY, 99 );

	ILuint ImageName = 0;
	ilGenImages( 1, &ImageName );
	ilBindImage( ImageName );

	ilTexImage( xsize, ysize, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL );
	ilSetData( buf );
	if ( filename.find( ".bmp" ) > 2 && filename.find( ".bmp" ) < 100 )
		ilSave( IL_BMP, filename.c_str() );
	else
		ilSave( IL_TGA, filename.c_str() );
	
	ilDeleteImages( 1, &ImageName );
	delete[] buf;
}

CBitmap
CBitmap::CreateMipmapLevel()
{
	int x,y,x2,y2;
	float r,g,b,a;
	int v1;

	CBitmap bm;
	delete[] bm.mem;

	bm.xsize = xsize / 2;
	bm.ysize = ysize / 2;
	bm.mem = new unsigned char[ bm.xsize * bm.ysize * 4 ];

	for ( y = 0; y < ysize / 2; ++y ) {
		for ( x = 0; x < xsize / 2; ++x ) {
			r = g = b = a = 0;
			for ( y2 = 0; y2 < 2; ++y2 ) {
				for ( x2 = 0; x2 < 2; ++x2 ) {
					v1 = ( (y * 2 + y2) * xsize + x * 2 + x2) * 4;
					r += mem[ v1     ];
					g += mem[ v1 + 1 ];
					b += mem[ v1 + 2 ];
					a += mem[ v1 + 3 ];
				}
			}
			bm.mem[ (y * bm.xsize + x) * 4     ] = (unsigned char)( r / 4 );
			bm.mem[ (y * bm.xsize + x) * 4 + 1 ] = (unsigned char)( g / 4 );
			bm.mem[ (y * bm.xsize + x) * 4 + 2 ] = (unsigned char)( b / 4 );
			bm.mem[ (y * bm.xsize + x) * 4 + 3 ] = (unsigned char)( a / 4 );
		}
	}
	return bm;
}

CBitmap
CBitmap::CreateRescaled( int newx, int newy )
{
	int x,y,x2,y2,ex,ey,sx,sy;
	float cx,cy,dx,dy;
	float r,g,b,a;

	CBitmap bm;
	delete[] bm.mem;

	bm.xsize = newx;
	bm.ysize = newy;
	bm.mem = new unsigned char[ bm.xsize * bm.ysize * 4 ];

	dx = float( xsize ) / newx;
	dy = float( ysize ) / newy;

	cy = 0;
	for ( y = 0; y < newy; ++y ) {
		sy = (int)cy;
		cy += dy;
		ey = (int)cy;
		if ( ey == sy) ey = sy + 1;

		cx = 0;
		for ( x = 0; x < newx; ++x ) {
			sx = (int)cx;
			cx += dx;
			ex = (int)cx;
			if ( ex == sx ) ex = sx + 1;

			r = g = b = a = 0;
			for ( y2 = sy; y2 < ey; ++y2 ) {
				for ( x2 = sx; x2 < ex; ++x2 ) {
					r += mem[ (y2 * xsize + x2) * 4 + 0 ];
					g += mem[ (y2 * xsize + x2) * 4 + 1 ];
					b += mem[ (y2 * xsize + x2) * 4 + 2 ];
					a += mem[ (y2 * xsize + x2) * 4 + 3 ];
				}
			}
			bm.mem[ (y * bm.xsize + x) * 4 + 0 ] = r / ((ex - sx) * (ey - sy));
			bm.mem[ (y * bm.xsize + x) * 4 + 1 ] = g / ((ex - sx) * (ey - sy));
			bm.mem[ (y * bm.xsize + x) * 4 + 2 ] = b / ((ex - sx) * (ey - sy));
			bm.mem[ (y * bm.xsize + x) * 4 + 3 ] = a / ((ex - sx) * (ey - sy));
		}	
	}
	return bm;
}

#define RM	0x0000F800
#define GM  0x000007E0
#define BM  0x0000001F

#define RED_RGB565(x) ((x&RM)>>11)
#define GREEN_RGB565(x) ((x&GM)>>5)
#define BLUE_RGB565(x) (x&BM)
#define PACKRGB(r, g, b) (((r<<11)&RM) | ((g << 5)&GM) | (b&BM) )

void
CBitmap::CreateFromDXT1( unsigned char *buf, int xsize, int ysize )
{
	int i,a,b;
	delete[] mem;

	mem = new unsigned char[ xsize * ysize * 4 ];
	memset( mem, 0, xsize * ysize * 4 );

	this->xsize = xsize;
	this->ysize = ysize;

	int numblocks = ((xsize + 3) / 4) * ((ysize + 3) / 4);

	unsigned char *temp = buf;

	for ( i = 0; i < numblocks; i++ ) {

		unsigned short color0 = (*(unsigned short *)&temp[ 0 ]);
		unsigned short color1 = (*(unsigned short *)&temp[ 2 ]);

		int r0 = RED_RGB565(   color0 ) << 3;
		int g0 = GREEN_RGB565( color0 ) << 2;
		int b0 = BLUE_RGB565(  color0 ) << 3;

		int r1 = RED_RGB565(   color1 ) << 3;
		int g1 = GREEN_RGB565( color1 ) << 2;
		int b1 = BLUE_RGB565(  color1 ) << 3;

		unsigned int bits = (*(unsigned int*)&temp[ 4 ]);
		
		for ( a = 0; a < 4; a++ ) {
			for ( b = 0; b < 4; b++ ) {
				int x = 4 * (i % ((xsize + 3) / 4)) + b;
				int y = 4 * (i / ((xsize + 3) / 4)) + a;
				bits >>= 2;
				unsigned char code = bits & 0x3;
				
				if ( color0 > color1 ) {
					if ( code == 0 ) {
						mem[ (y * xsize + x) * 4 + 0 ] = r0;
						mem[ (y * xsize + x) * 4 + 1 ] = g0;
						mem[ (y * xsize + x) * 4 + 2 ] = b0;
					} else if ( code == 1 ) {
						mem[ (y * xsize + x) * 4 + 0 ] = r1;
						mem[ (y * xsize + x) * 4 + 1 ] = g1;
						mem[ (y * xsize + x) * 4 + 2 ] = b1;
					} else if ( code == 2 ) {
						mem[ (y * xsize + x) * 4 + 0 ] = (r0 * 2 + r1) / 3;
						mem[ (y * xsize + x) * 4 + 1 ] = (g0 * 2 + g1) / 3;
						mem[ (y * xsize + x) * 4 + 2 ] = (b0 * 2 + b1) / 3;
					} else {	
						mem[ (y * xsize + x) * 4 + 0 ] = (r0 + r1 * 2) / 3;
						mem[ (y * xsize + x) * 4 + 1 ] = (g0 + g1 * 2) / 3;
						mem[ (y * xsize + x) * 4 + 2 ] = (b0 + b1 * 2) / 3;
					}
				} else {
					if ( code == 0 ) {
						mem[ (y * xsize + x) * 4 + 0 ] = r0;
						mem[ (y * xsize + x) * 4 + 1 ] = g0;
						mem[ (y * xsize + x) * 4 + 2 ] = b0;
					} else if ( code == 1 ) {
						mem[ (y * xsize + x) * 4 + 0 ] = r1;
						mem[ (y * xsize + x) * 4 + 1 ] = g1;
						mem[ (y * xsize + x) * 4 + 2 ] = b1;
					} else if ( code == 2 ) {
						mem[ (y * xsize + x) * 4 + 0 ] = (r0 + r1) / 2;
						mem[ (y * xsize + x) * 4 + 1 ] = (g0 + g1) / 2;
						mem[ (y * xsize + x) * 4 + 2 ] = (b0 + b1) / 2;
					} else {	
						mem[ (y * xsize + x) * 4 + 0 ] = 0;
						mem[ (y * xsize + x) * 4 + 1 ] = 0;
						mem[ (y * xsize + x) * 4 + 2 ] = 0;
					}
				}
			}
		}
		temp += 8;
	}
}
