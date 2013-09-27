#ifndef __TILEHANDLER_H__
#define __TILEHANDLER_H__

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include "Bitmap.h"

using namespace std;

#define MAX_MAP_SIZE 40
#define MAX_TILES (MAX_MAP_SIZE*MAX_MAP_SIZE*16*16)

class CTileHandler
{
public:
	CTileHandler();
	~CTileHandler( void );

	void LoadTexture( string name );
	void SaveData( ofstream &ofs );
	void ReadTile( int xpos, int ypos, char *destbuf, char *sourcebuf );
	int  FindCloseTile( CBitmap* bm, int forbidden );

	void ProcessTiles( float compressFactor, bool fastcompress );
	void ProcessTiles2( void );

	int  GetFileSize( void );

	void AddExternalTileFile( string file );
	void SetOutputFile( string file );
	bool CompareTiles( CBitmap *bm, CBitmap *bm2 );

	CBitmap bigTex;
	int xsize;
	int ysize;

	CBitmap* tiles[ MAX_TILES ];
	vector<char *> newTiles;
	int tileUse[ MAX_TILES ];
	int usedTiles;
	int numExternalTile;

	struct FastStat{
		int r,g,b;
		int rx,gx,bx;
		int ry,gy,by;
	};
	FastStat fastStats[ MAX_TILES ];
	FastStat CalcFastStat( CBitmap* bm );

	int meanThreshold;
	int meanDirThreshold;
	int borderThreshold;

	vector<string> externalFiles;
	vector<int> externalFileTileSize;

	string myTileFile;
};

extern CTileHandler tileHandler;

#endif // __TILEHANDLER_H__
