// Bitmap.h: interface for the CBitmap class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <string>
#include <IL/il.h>

using std::string;

class CBitmap  
{
public:
	CBitmap(unsigned char* data,int xsize,int ysize);
	CBitmap(string const& filename);
	CBitmap();
	CBitmap(const CBitmap& old);
	CBitmap& operator=(const CBitmap& bm);

	virtual ~CBitmap();
	

	void Load(string const& filename, unsigned char defaultAlpha=255, bool verylarge=false);
	void Save(string const& filename);

	unsigned int CreateTexture(bool mipmaps=false);
	unsigned int CreateDDSTexture();

	void CreateAlpha(unsigned char red,unsigned char green,unsigned char blue);
	void SetTransparent(unsigned char red, unsigned char green, unsigned char blue);

	CBitmap GetRegion(int startx, int starty, int width, int height);
	CBitmap CreateMipmapLevel(void);

	unsigned char* mem;
	int xsize;
	int ysize;
	ILuint vl;//for very large images.

public:
	CBitmap CreateRescaled(int newx, int newy);
	void ReverseYAxis(void);
	void CreateFromDXT1(unsigned char* buf, int xsize, int ysize);
};

#endif // __BITMAP_H__
