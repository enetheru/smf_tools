#pragma once

#include <string>
#include <fstream>
#include "bitmap.h"

using namespace std;

class CTileHandler
{
public:
	CTileHandler(void);
	~CTileHandler(void);
	void LoadTexture(string name);
	void ProcessTiles(float compressFactor);
	void SaveData(ofstream& ofs);
	void ReadTile(int xpos, int ypos, char *destbuf, char *sourcebuf);
	int FindCloseTile(CBitmap* bm,int forbidden);
	void ProcessTiles2(void);

	CBitmap bigTex;
	int xsize;
	int ysize;

	CBitmap* tiles[200000];//increase maybe
	int tileUse[200000];
	int usedTiles;
	
	struct FastStat{
		int r,g,b;
		int rx,gx,bx;
		int ry,gy,by;
	};
	FastStat fastStats[200000];
	FastStat CalcFastStat(CBitmap* bm);
	bool CompareTiles(CBitmap* bm, CBitmap* bm2);

	int meanThreshold;
	int meanDirThreshold;
	int borderThreshold;
};

extern CTileHandler tileHandler;