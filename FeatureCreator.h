#pragma once

#include <fstream>
#include <vector>
#include "../rts/mapfile.h"
#include "bitmap.h"

using namespace std;

class CFeatureCreator
{
public:
	CFeatureCreator(void);
	~CFeatureCreator(void);
	void WriteToFile(ofstream* file, vector<string> F_map);

	std::vector<MapFeatureStruct> features;
	void CreateFeatures(CBitmap* bm, int startx, int starty,std::string metalfile, int arbFeatureTypes, std::string featurefile);

	int xsize,ysize;

	unsigned char* vegMap;
};
