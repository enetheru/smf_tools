// MapConv.cpp : Defines the entry point for the console application.

#include "Bitmap.h"
#include <string>
#include <stdio.h>
#include "mapfile.h"
#include <fstream>
#include "FileHandler.h"
#include <math.h>
#include <string.h>
#ifdef WIN32
#include "ddraw.h"
#endif
#include "FeatureCreator.h"
#include "TileHandler.h"
#include "tclap/CmdLine.h"
#include <vector>
#ifndef WIN32
#include "time.h" /* time() */
#endif

using namespace std;
using namespace TCLAP;

CFeatureCreator featureCreator;
void ConvertTextures(string intexname,string temptexname,int xsize,int ysize);
void LoadHeightMap(string inname,int xsize,int ysize,float minHeight,float maxHeight,bool invert,bool lowpass);
void SaveHeightMap(ofstream& outfile,int xsize,int ysize,float minHeight,float maxHeight);
void SaveTexOffsets(ofstream &outfile,string temptexname,int xsize,int ysize);
void SaveTextures(ofstream &outfile,string temptexname,int xsize,int ysize);

void SaveMiniMap(ofstream &outfile);
void SaveMetalMap(ofstream &outfile, std::string metalmap, int xsize, int ysize);
void SaveTypeMap(ofstream &outfile,int xsize,int ysize,string typemap);
void MapFeatures(const char *ffile, char *F_Array);
float* heightmap;
#ifndef WIN32
string stupidGlobalCompressorName;
#endif

#ifdef WIN32
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char ** argv)
#endif
{
	int xsize;
	int ysize;
	string intexname="mars.bmp";
	string inHeightName="mars.raw";
	string outfilename="mars.smf";
	string metalmap="marsmetal.bmp";
	string typemap="";
	string extTileFile="";
	string featuremap="";
	string geoVentFile="geovent.bmp";
	string featureListFile="fs.txt";
	float minHeight=20;
	float maxHeight=300;
	float compressFactor=0.8f;
	float whereisit=0;
	bool invertHeightMap=false;
	bool lowpassFilter=false;
	vector<string> F_Spec;

	try {
		// Define the command line object.
		CmdLine cmd(
			"Converts a series of image files to a Spring map. This just creates the .smf and .smt files. You also need to write a .smd file using a text editor.",
			' ', "0.5");

		// Define a value argument and add it to the command line.
		ValueArg<string> intexArg("t", "intex",
			"Input bitmap to use for the map. Sides must be multiple of 1024 long. xsize, ysize determined from this file: xsize = intex width / 8, ysize = height / 8.",
			true, "test.bmp", "texturemap file");
		cmd.add( intexArg );
		ValueArg<string> heightArg("a", "heightmap",
			"Input heightmap to use for the map, this should be in 16 bit raw format (.raw extension) or an image file. Must be xsize*64+1 by ysize*64+1.",
			true, "test.raw", "heightmap file");
		cmd.add( heightArg );
		ValueArg<string> metalArg("m", "metalmap",
			"Metal map to use, red channel is amount of metal. Resized to xsize / 2 by ysize / 2.",
			true, "metal.bmp", "metalmap image");
		cmd.add( metalArg );
		ValueArg<string> typeArg("y","typemap",
			"Type map to use, uses the red channel to decide type. types are defined in the .smd, if this argument is skipped the map will be all type 0",
			false, "", "typemap image");
		cmd.add( typeArg );
		ValueArg<string> geoArg("g","geoventfile",
			"The decal for geothermal vents; appears on the compiled map at each vent. (Default: geovent.bmp).",
			false, "geovent.bmp", "Geovent image");
		cmd.add( geoArg );
		ValueArg<string> tileArg("e", "externaltilefile",
			"External tile file that will be used for finding tiles. Tiles not found in this will be saved in a new tile file.",
			false, "", "tile file");
		cmd.add( tileArg );
		ValueArg<string> outArg("o", "outfile",
			"The name of the created map file. Should end in .smf. A tilefile (extension .smt) is also created.",
			false, "test.smf", "output .smf");
		cmd.add( outArg );
		ValueArg<float> minhArg("n", "minheight",
			"What altitude in spring the min(0) level of the height map represents.",
			true, -20, "min height");
		cmd.add( minhArg );
		ValueArg<float> maxhArg("x", "maxheight",
			"What altitude in spring the max(0xff or 0xffff) level of the height map represents.",
			true, 500, "max height");
		cmd.add( maxhArg );
		ValueArg<float> compressArg("c", "compress",
			"How much we should try to compress the texture map. Default 0.8, lower -> higher quality, larger files.",
			false, 0.8f, "compression");
		cmd.add( compressArg );
#ifndef WIN32
		ValueArg<string> texCompressArg("z", "texcompress",
			"Name of companion program texcompress from current working directory.",
			false, "./texcompress", "texcompress program");
		cmd.add( texCompressArg );
#endif

		// Actually, it flips the heightmap *after* it's been read in. Hopefully this is clearer.
		SwitchArg invertSwitch("i", "invert",
			"Flip the height map image upside-down on reading.",
			false);
		cmd.add( invertSwitch );
		SwitchArg lowpassSwitch("l", "lowpass",
			"Lowpass filters the heightmap",
			false);
		cmd.add( lowpassSwitch );
		ValueArg<string> featureArg("f", "featuremap",
			"Feature placement file, xsize by ysize. See README.txt for details.",
			false, "", "featuremap image");
		cmd.add( featureArg );
		ValueArg<string> featureListArg("j", "featurelist",
			"A file with the name of one feature on each line. (Default: fs.txt). See README.txt for details.",
			false, "fs.txt", "feature list file");
		cmd.add( featureListArg );

		// Parse the args.
		cmd.parse( argc, argv );

		// Get the value parsed by each arg.
		intexname=intexArg.getValue();
		inHeightName=heightArg.getValue();
		outfilename=outArg.getValue();
		typemap=typeArg.getValue();
		extTileFile=tileArg.getValue();
		metalmap=metalArg.getValue();
		minHeight=minhArg.getValue();
		maxHeight=maxhArg.getValue();
		compressFactor=compressArg.getValue();
		invertHeightMap=invertSwitch.getValue();
		lowpassFilter=lowpassSwitch.getValue();
		featuremap=featureArg.getValue();
		geoVentFile=geoArg.getValue();
		featureListFile=featureListArg.getValue();
#ifndef WIN32
		stupidGlobalCompressorName=texCompressArg.getValue();
#endif
	} catch (ArgException &e)  // catch any exceptions
	{ cerr << "error: " << e.error() << " for arg " << e.argId() << endl; exit(-1);}

	tileHandler.LoadTexture(intexname);
	tileHandler.SetOutputFile(outfilename);
	if(!extTileFile.empty())
		tileHandler.AddExternalTileFile(extTileFile);

	xsize=tileHandler.xsize;
	ysize=tileHandler.ysize;

	LoadHeightMap(inHeightName,xsize,ysize,minHeight,maxHeight,invertHeightMap,lowpassFilter);

	ifstream ifs;
	int numNamedFeatures=0;

	ifs.open(featureListFile.c_str(), ifstream::in);
	while (ifs.good()){
			char c[100]="";
			ifs.getline(c,100);
			F_Spec.push_back(c);
			numNamedFeatures++;
	}
	featureCreator.CreateFeatures(&tileHandler.bigTex,0,0,numNamedFeatures,featuremap,geoVentFile);

	tileHandler.ProcessTiles(compressFactor);

#ifdef WIN32
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
#endif

	MapHeader header;
	strcpy(header.magic,"spring map file");
	header.version=1;
#ifdef WIN32
	header.mapid=li.LowPart;		//todo: this should be made better to make it depend on heightmap etc, but this should be enough to make each map unique
#else
	header.mapid = time(NULL);
#endif
	header.mapx=xsize;
	header.mapy=ysize;
	header.squareSize=8;
	header.texelPerSquare=8;
	header.tilesize=32;
	header.minHeight=minHeight;
	header.maxHeight=maxHeight;

	header.numExtraHeaders=1;

	int headerSize=sizeof(MapHeader);
	headerSize+=12;		//size of vegetation extra header

	header.heightmapPtr = headerSize;
	header.typeMapPtr= header.heightmapPtr + (xsize+1)*(ysize+1)*2;
	header.minimapPtr = header.typeMapPtr + (xsize/2) * (ysize/2);
	header.tilesPtr = header.minimapPtr + MINIMAP_SIZE;
	header.metalmapPtr = header.tilesPtr + tileHandler.GetFileSize();
	header.featurePtr = header.metalmapPtr + (xsize/2)*(ysize/2) + (xsize/4*ysize/4);	//last one is space for vegetation map


	ofstream outfile(outfilename.c_str(), ios::out|ios::binary);

	outfile.write((char*)&header,sizeof(MapHeader));

	int temp=12;	//extra header size
	outfile.write((char*)&temp,4);
	temp=MEH_Vegetation;	//extra header type
	outfile.write((char*)&temp,4);
	temp=header.metalmapPtr + (xsize/2)*(ysize/2);		//offset to vegetation map
	outfile.write((char*)&temp,4);

	SaveHeightMap(outfile,xsize,ysize,minHeight,maxHeight/*,whereisit*/);

	SaveTypeMap(outfile,xsize,ysize,typemap);
	SaveMiniMap(outfile);

	tileHandler.ProcessTiles2();
	tileHandler.SaveData(outfile);

	SaveMetalMap(outfile, metalmap,xsize,ysize);

	featureCreator.WriteToFile(&outfile, F_Spec);

	delete[] heightmap;
	return 0;
}

void SaveMiniMap(ofstream &outfile)
{
	printf("creating minimap\n");

	CBitmap mini = tileHandler.bigTex.CreateRescaled(1024, 1024);
	mini.Save("mini.bmp");
#ifdef WIN32
	system("nvdxt.exe -file mini.bmp -dxt1c -dither");

	DDSURFACEDESC2 ddsheader;
	int ddssignature;

	CFileHandler file("mini.dds");
	file.Read(&ddssignature, sizeof(int));
	file.Read(&ddsheader, sizeof(DDSURFACEDESC2));
#else
	char execstring[512];
	snprintf(execstring, 512, "%s mini.bmp", stupidGlobalCompressorName.c_str());
	system(execstring);
	CFileHandler file("mini.bmp.raw");
#endif

	char minidata[MINIMAP_SIZE];
	file.Read(minidata, MINIMAP_SIZE);

	outfile.write(minidata, MINIMAP_SIZE);
}

void LoadHeightMap(string inname,int xsize,int ysize,float minHeight,float maxHeight,bool invert,bool lowpass)
{
	printf("Creating height map\n");

	float hDif=maxHeight-minHeight;
	int mapx=xsize+1;
	int mapy=ysize+1;
	heightmap=new float[mapx*mapy];

	if(inname.find(".raw")!=string::npos){		//16 bit raw
		CFileHandler fh(inname);

		for(int y=0;y<mapy;++y){
			for(int x=0;x<mapx;++x){
				unsigned short h;
				fh.Read(&h,2);
				heightmap[(ysize-y)*mapx+x]=(float(h))/65535*hDif+minHeight;
			}
		}
	} else {		//standard image
		CBitmap bm(inname);
		if(bm.xsize!=mapx || bm.ysize!=mapy){
			printf("Errenous dimensions for heightmap image");
			exit(0);
		}
		for(int y=0;y<mapy;++y){
			for(int x=0;x<mapx;++x){
				unsigned short h=(int)bm.mem[(y*mapx+x)*4]*256;
				heightmap[(ysize-y)*mapx+x]=(float(h))/65535*hDif+minHeight;
			}
		}
	}

	if(invert){
		float* heightmap2=heightmap;
		heightmap=new float[mapx*mapy];
		for(int y=0;y<mapy;++y){
			for(int x=0;x<mapx;++x){
				heightmap[y*mapx+x]=heightmap2[(mapy-y-1)*mapx+x];
			}
		}
		delete[] heightmap2;
	}

	if(lowpass){
		float* heightmap2=heightmap;
		heightmap=new float[mapx*mapy];
		for(int y=0;y<mapy;++y){
			for(int x=0;x<mapx;++x){
				float h=0;
				float tmod=0;
				for(int y2=max(0,y-2);y2<min(mapy,y+3);++y2){
					int dy=y2-y;
					for(int x2=max(0,x-2);x2<min(mapx,x+3);++x2){
						int dx=x2-x;
						float mod=max(0.0f,1.0f-0.4f*sqrtf(float(dx*dx+dy*dy)));
						tmod+=mod;
						h+=heightmap2[y2*mapx+x2]*mod;
					}
				}
				heightmap[y*mapx+x]=h/tmod;
			}
		}
		delete[] heightmap2;
	}
}

void SaveHeightMap(ofstream& outfile,int xsize,int ysize,float minHeight,float maxHeight)
{
	int mapx=xsize+1;
	int mapy=ysize+1;
	unsigned short* hm=new unsigned short[mapx*mapy];
	float sub=minHeight;
	float mul=(1.0f/(maxHeight-minHeight))*0xffff;
	for(int y=0;y<mapy;++y){
		for(int x=0;x<mapx;++x){
			hm[y*mapx+x]=(unsigned short)((heightmap[y*mapx+x]-sub)*mul);
		}
	}
	outfile.write((char*)hm,mapx*mapy*2);

	delete[] hm;
}

void SaveMetalMap(ofstream &outfile, std::string metalmap, int xsize, int ysize)
{
	printf("Saving metal map\n");

	CBitmap metal(metalmap);
	if(metal.xsize!=xsize/2 || metal.ysize!=ysize/2){
		metal=metal.CreateRescaled(xsize/2,ysize/2);
	}
	int size = (xsize/2)*(ysize/2);
	char *buf = new char[size];

	for(int y=0;y<metal.ysize;++y)
		for(int x=0;x<metal.xsize;++x)
			buf[y*metal.xsize+x]=metal.mem[(y*metal.xsize+x)*4];	//we use the red component of the picture

	outfile.write(buf, size);

	delete [] buf;
}

void SaveTypeMap(ofstream &outfile,int xsize,int ysize,string typemap)
{
	int mapx=xsize/2;
	int mapy=ysize/2;

	unsigned char* typeMapMem=new unsigned char[mapx*mapy];
	memset(typeMapMem,0,mapx*mapy);

	if(!typemap.empty()){
		CBitmap tm;
		tm.Load(typemap);
		CBitmap tm2=tm.CreateRescaled(mapx,mapy);
		for(int a=0;a<mapx*mapy;++a)
			typeMapMem[a]=tm2.mem[a*4];
	}
	outfile.write((char*)typeMapMem,mapx*mapy);

	delete[] typeMapMem;
}

