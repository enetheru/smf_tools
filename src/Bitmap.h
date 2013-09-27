// Bitmap.h: interface for the CBitmap class.
#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <string>
#include <IL/il.h>

using std::string;

class CBitmap  
{
public:
	//constructors
	CBitmap();
	CBitmap( unsigned char *data, int xsize, int ysize );
	CBitmap( string const &filename );
	CBitmap( const CBitmap &old );
	// Operators
	CBitmap &operator=( const CBitmap& bm );
	// Destructor
	virtual ~CBitmap();

	//Functions
	void Load( string const &filename, unsigned char defaultAlpha = 255 );
	void Save( string const &filename );

	unsigned int CreateDDSTexture();

	CBitmap CreateMipmapLevel();

	//Variables
	unsigned char* mem;
	int xsize;
	int ysize;

public:
	CBitmap CreateRescaled( int newx, int newy );
	void CreateFromDXT1( unsigned char *buf, int xsize, int ysize );
};

#endif // __BITMAP_H__
