#pragma once

#include <fstream>
#include <vector>
#include "sm2header.h"
#include "bitmap.h"

using namespace std;

class CFeatureCreator
{
public:
	CFeatureCreator(void);
	~CFeatureCreator(void);
	void WriteToFile(ofstream* file);

	std::vector<FeatureFileStruct> features;
	void CreateFeatures(CBitmap* bm, int startx, int starty,std::string metalfile);
};
