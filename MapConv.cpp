// MapConv.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "bitmap.h"
#include <string>
#include <stdio.h>
#include "sm2header.h"
#include <fstream>
#include "filehandler.h"
#include "ddraw.h"
#include "featurecreator.h"
#include "tilehandler.h"
#include "tclap/CmdLine.h"

using namespace std;
using namespace TCLAP;

CFeatureCreator featureCreator;
void ConvertTextures(string intexname,string temptexname,int xsize,int ysize);
void LoadHeightMap(string inname,int xsize,int ysize,float minHeight,float maxHeight,bool invert,bool lowpass);
void SaveHeightMap(ofstream& outfile,int xsize,int ysize);
void SaveTexOffsets(ofstream &outfile,string temptexname,int xsize,int ysize);
void SaveTextures(ofstream &outfile,string temptexname,int xsize,int ysize);

void SaveMiniMap(ofstream &outfile);
void SaveMetalMap(ofstream &outfile, std::string metalmap, int xsize, int ysize);

float* heightmap;

int _tmain(int argc, _TCHAR* argv[])
{
	int xsize;
	int ysize;
	string intexname="mars.bmp";
	string inHeightName="mars.raw";
	string outfilename="mars.sm2";
	string metalmap="marsmetal.bmp";
	float minHeight=20;
	float maxHeight=300;
	float compressFactor=0.8f;
	bool invertHeightMap=false;
	bool lowpassFilter=false;

	try {  
		// Define the command line object.
		CmdLine cmd("Convers a series of bmp/raw files to a spring map. This just creates the .sm2 file, you will also have to create a .smd file using a text editor", ' ', "0.5");

		// Define a value argument and add it to the command line.
		ValueArg<string> intexArg("t","intex","Input bitmap to use for the map. Must be xsize*1024 x ysize*1024 in size. (xsize,ysize integers determined from this file)",true,"test.bmp","Texture file");
		cmd.add( intexArg );
		ValueArg<string> heightArg("a","heightmap","Input heightmap to use for the map, this should be in 16 bit raw format (.raw extension) or an image file. Must be xsize*128+1 x ysize*128+1 in size.",true,"test.raw","Heightmap file");
		cmd.add( heightArg );
		ValueArg<string> metalArg("m","metalmap","Metal map to use, red channel is amount of metal, green=255 is where to place geos (one pixel=one geo). Is resized to fit the mapsize",true,"metal.bmp","Metalmap file");
		cmd.add( metalArg );
		ValueArg<string> outArg("o","outfile","The name of the created map",false,"test.sm2","Output file");
		cmd.add( outArg );
		ValueArg<float> minhArg("n","minheight","What altitude in spring the min(0) level of the height map should represent",true,-20,"min height");
		cmd.add( minhArg );
		ValueArg<float> maxhArg("x","maxheight","What altitude in spring the max(0xffff) level of the height map should represent",true,500,"max height");
		cmd.add( maxhArg );
		ValueArg<float> compressArg("c","compress","How much we should try to compress the texture file. Default value 0.8 . (lower=higher quality, larger file)",false,0.8f,"compression");
		cmd.add( compressArg );

		SwitchArg invertSwitch("i","invert","Inverts the height map vertically (not the height values but the height map (uhm someone rewrite this))", false);
		cmd.add( invertSwitch );
		SwitchArg lowpassSwitch("l","lowpass","Lowpass filters the heightmap", false);
		cmd.add( lowpassSwitch );

		// Parse the args.
		cmd.parse( argc, argv );

		// Get the value parsed by each arg. 
		intexname=intexArg.getValue();
		inHeightName=heightArg.getValue();
		outfilename=outArg.getValue();
		metalmap=metalArg.getValue();
		minHeight=minhArg.getValue();
		maxHeight=maxhArg.getValue();
		compressFactor=compressArg.getValue();
		invertHeightMap=invertSwitch.getValue();
		lowpassFilter=lowpassSwitch.getValue();
	} catch (ArgException &e)  // catch any exceptions
	{ cerr << "error: " << e.error() << " for arg " << e.argId() << endl; exit(-1);}

	tileHandler.LoadTexture(intexname);
	xsize=tileHandler.xsize;
	ysize=tileHandler.ysize;

	LoadHeightMap(inHeightName,xsize,ysize,minHeight,maxHeight,invertHeightMap,lowpassFilter);

	featureCreator.CreateFeatures(&tileHandler.bigTex,0,0,metalmap);

	tileHandler.ProcessTiles(compressFactor);


	SM2Header header;
	strcpy(header.magic,"SM2File");
	header.version=1;
	header.xsize=xsize;
	header.ysize=ysize;
	header.tilesize=32;
	header.numtiles=tileHandler.usedTiles;

	header.heightmapPtr = sizeof(SM2Header);
	header.minimapPtr = header.heightmapPtr + (xsize+1)*(ysize+1)*4;
	header.tilemapPtr = header.minimapPtr + MINIMAP_SIZE;
	header.tilesPtr = header.tilemapPtr + ((xsize*ysize)/(4*4))*4;
	header.metalmapPtr = header.tilesPtr + tileHandler.usedTiles*SMALL_TILE_SIZE;
	header.featurePtr = header.metalmapPtr + (xsize/2)*(ysize/2);
	
	
	ofstream outfile(outfilename.c_str(), ios::out|ios::binary);

	outfile.write((char*)&header,sizeof(SM2Header));

	SaveHeightMap(outfile,xsize,ysize);

	SaveMiniMap(outfile);

	tileHandler.ProcessTiles2();
	tileHandler.SaveData(outfile);

	SaveMetalMap(outfile, metalmap,xsize,ysize);

	featureCreator.WriteToFile(&outfile);

	return 0;
}

void SaveMiniMap(ofstream &outfile)
{
	printf("creating minimap\n");

	CBitmap mini = tileHandler.bigTex.CreateRescaled(1024, 1024);
	mini.Save("mini.bmp");
	system("nvdxt.exe -file mini.bmp -dxt1c -dither");

	DDSURFACEDESC2 ddsheader;
	int ddssignature;

	CFileHandler file("mini.dds");
	file.Read(&ddssignature, sizeof(int));
	file.Read(&ddsheader, sizeof(DDSURFACEDESC2));

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
				for(int y2=max(0,y-2);y2<min(mapx,y+3);++y2){
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
/*
	int bfsizex=xsize/2;
	int bfsizey=ysize/2;

	float* bfmap=new float[bfsizex*bfsizey];


	for(int y=0;y<bfsizey;++y){
		for(int x=0;x<bfsizex;++x){
			unsigned short h;
			fh.Read(&h,2);
			bfmap[y*bfsizex+x]=float(h)/128;
		}
	}


	for(int x=0; x<bfsizex; x++){
		for(int y=0; y<bfsizey; y++){
			heightmap[(y*2)*mapx+(x*2)]=bfmap[y*bfsizex+x];
		}
	}
	for(int x=0; x<bfsizex; x++){
		heightmap[(ysize)*mapx+(x*2)]=bfmap[(bfsizey-1)*bfsizex+x];
	}
	for(int y=0; y<bfsizey; y++){
		heightmap[(y*2)*mapx+(xsize)]=bfmap[y*bfsizex+bfsizex-1];
	}
	heightmap[ysize*xsize+xsize]=heightmap[ysize*xsize+xsize-2];

	for(int y=0;y<mapy/2;++y){
		for(int x=0;x<mapx/2;++x){

			float h=heightmap[(y*2)*mapx+(x*2)]*0.57f+heightmap[(y*2)*mapx+(x+1)*2]*0.57f;
			h-=heightmap[(y*2)*mapx+max(0,x-1)*2]*0.07f+heightmap[(y*2)*mapx+min(mapx-1,(x+2)*2)]*0.07f;
			heightmap[(y*2)*mapx+x*2+1]=h;

			h=heightmap[(y*2)*mapx+x*2]*0.57f+heightmap[(y+1)*2*mapx+x*2]*0.57f;
			h-=heightmap[max(0,(y-1)*2)*mapx+x*2]*0.07f + heightmap[min(mapy-1,(y+2)*2)*mapx+x*2]*0.07f;
			heightmap[(y*2+1)*mapx+x*2]=h;
		}
	}

	for(int y=0;y<mapy/2;++y){
		int x=mapx/2;
		float h=heightmap[(y*2)*mapx+x*2]*0.57f+heightmap[(y+1)*2*mapx+x*2]*0.57f;
		h-=heightmap[max(0,(y-1)*2)*mapx+x*2]*0.07f + heightmap[min(mapy-1,(y+2)*2)*mapx+x*2]*0.07f;
		heightmap[(y*2+1)*mapx+x*2]=h;
	}	

	for(int x=0;x<mapx/2;++x){
		int y=mapy/2;
		float h=heightmap[(y*2)*mapx+(x*2)]*0.57f+heightmap[(y*2)*mapx+(x+1)*2]*0.57f;
		h-=heightmap[(y*2)*mapx+max(0,x-1)*2]*0.07f+heightmap[(y*2)*mapx+min(mapx-1,(x+2)*2)]*0.07f;
		heightmap[(y*2)*mapx+x*2+1]=h;
	}

	for(int y=0;y<mapy/2;++y){
		for(int x=0;x<mapx/2;++x){
			float hx=heightmap[(y*2+1)*mapx+x*2]*0.57f+heightmap[(y*2+1)*mapx+x*2+2]*0.57f;
			hx-=heightmap[(y*2+1)*mapx+max(0,x*2-2)]*0.07f+heightmap[(y*2+1)*mapx+min(mapx-1,x*2+4)]*0.07f;

			float hy=heightmap[(y*2)*mapx+x*2+1]*0.57f+heightmap[(y*2+2)*mapx+x*2+1]*0.57f;
			hy-=heightmap[max(0,(y*2-2))*mapx+x*2+1]*0.07f+heightmap[min(mapy-1,(y*2+4))*mapx+x*2+1]*0.07f;

			heightmap[(y*2+1)*mapx+x*2+1]=(hx+hy)*0.5f;
		}
	}
*/
}

void SaveHeightMap(ofstream& outfile,int xsize,int ysize)
{
	outfile.write((char*)heightmap,(xsize+1)*(ysize+1)*4);

	delete[] heightmap;
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