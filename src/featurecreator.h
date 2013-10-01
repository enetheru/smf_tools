#ifndef __FEATURECREATOR_H__
#define __FEATURECREATOR_H__

#include <fstream>
#include <vector>
#include "mapfile.h"
#include "bitmap.h"
#include "string.h"

using namespace std;
struct LuaFeature{
	short int rot;
	int posx;
	int posz;
	string name;
};
class CFeatureCreator
{
public:
	CFeatureCreator(void);
	~CFeatureCreator(void);
	void WriteToFile(ofstream* file, vector<string> F_map);
	void CreateFeatures(CBitmap* bm, int startx, int starty, int arbFeatureTypes, std::string featurefile, std::string geoVentFile, vector<LuaFeature> lf, vector<string> F_Spec);
	
private:
	int xsize,ysize;
	int mapx;

	std::vector<MapFeatureStruct> features;

	unsigned char* vegMap;

	void PlaceVent(int x, int y, CBitmap * feature, CBitmap * vent, CBitmap * bm);
	bool FlatSpot(int x, int y);
};

#endif // __FEATURECREATOR_H__
