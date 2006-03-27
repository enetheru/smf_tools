// MapConv.cpp : Defines the entry point for the console application.
// (please ignore all the yucky commented out stuff :) thx -mother

#include "stdafx.h"
#include "bitmap.h"
#include <string>
#include <stdio.h>
#include "../rts/mapfile.h"
#include <fstream>
#include "filehandler.h"
#include "ddraw.h"
#include "featurecreator.h"
#include "tilehandler.h"
#include "tclap/CmdLine.h"
#include <vector>

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

int _tmain(int argc, _TCHAR* argv[])
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
	float minHeight=20;
	float maxHeight=300;
	float compressFactor=0.8f;
	float whereisit=0;
	bool invertHeightMap=false;
	bool lowpassFilter=false;
	vector<string> F_Spec;

	try {  
		// Define the command line object.
		CmdLine cmd("Convers a series of bmp/raw files to a spring map. This just creates the .smf file, you will also have to create a .smd file using a text editor", ' ', "0.5");

		// Define a value argument and add it to the command line.
		ValueArg<string> intexArg("t","intex","Input bitmap to use for the map. Must be xsize*1024 x ysize*1024 in size. (xsize,ysize integers determined from this file)",true,"test.bmp","Texture file");
		cmd.add( intexArg );
		ValueArg<string> heightArg("a","heightmap","Input heightmap to use for the map, this should be in 16 bit raw format (.raw extension) or an image file. Must be xsize*128+1 x ysize*128+1 in size.",true,"test.raw","Heightmap file");
		cmd.add( heightArg );
		ValueArg<string> metalArg("m","metalmap","Metal map to use, red channel is amount of metal, green=255 is where to place geos (one pixel=one geo). Is resized to fit the mapsize",true,"metal.bmp","Metalmap file");
		cmd.add( metalArg );
		ValueArg<string> typeArg("y","typemap","Type map to use, uses the red channel to decide type, types are defined in the .smd, if this argument is skipped the map will be all type 0",false,"","Typemap file");
		cmd.add( typeArg );
		ValueArg<string> tileArg("e","externaltilefile","External tile file that will be used for finding tiles, tiles not found in this will be added in a new tile file",false,"","tile file");
		cmd.add( tileArg );
		ValueArg<string> outArg("o","outfile","The name of the created map",false,"test.smf","Output file");
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
		ValueArg<string> featureArg("f","featuremap","Feature placement file", false,"","Feature Placement");
		cmd.add( featureArg );
		//ValueArg<float> whereArg("w","whereisit","Where is this height?",true,0,"Where iddit?");
		//cmd.add( whereArg );

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
		//whereisit=whereArg.getValue();
	} catch (ArgException &e)  // catch any exceptions
	{ cerr << "error: " << e.error() << " for arg " << e.argId() << endl; exit(-1);}

/*	if(!extTileFile.empty())
		tileHandler.AddExternalTileFile(extTileFile);
	tileHandler.ProcessTiles(compressFactor);
*/
	tileHandler.LoadTexture(intexname);
	tileHandler.SetOutputFile(outfilename);
	if(!extTileFile.empty())
		tileHandler.AddExternalTileFile(extTileFile);

	xsize=tileHandler.xsize;
	ysize=tileHandler.ysize;

	LoadHeightMap(inHeightName,xsize,ysize,minHeight,maxHeight,invertHeightMap,lowpassFilter);
	//BOO
	ifstream ifs;
	int ex=0;

	//if (featureSpecFile!=""){
	//	ifs.open(featureSpecFile.c_str(), ifstream::in);
	ifs.open("fs.txt", ifstream::in);
	while (ifs.good()){
			char c[100]="";
		    ifs.getline(c,100);
			F_Spec.push_back(c);
			ex++;
	}
	//endboo
	featureCreator.CreateFeatures(&tileHandler.bigTex,0,0,metalmap,ex,featuremap);

	tileHandler.ProcessTiles(compressFactor);

	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);

	MapHeader header;
	strcpy(header.magic,"spring map file");
	header.version=1;
	header.mapid=li.LowPart;		//todo: this should be made better to make it depend on heightmap etc, but this should be enough to make each map unique
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

	SaveHeightMap(outfile,xsize,ysize,minHeight,maxHeight,whereisit);

	SaveTypeMap(outfile,xsize,ysize,typemap);
	SaveMiniMap(outfile);

	tileHandler.ProcessTiles2();
	tileHandler.SaveData(outfile);

	SaveMetalMap(outfile, metalmap,xsize,ysize);

	featureCreator.WriteToFile(&outfile, F_Spec);

	delete[] heightmap;
	//cout<<"Short:"<<sizeof(short);
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

void SaveHeightMap(ofstream& outfile,int xsize,int ysize,float minHeight,float maxHeight)
{
	int mapx=xsize+1;
	int mapy=ysize+1;
	unsigned short* hm=new unsigned short[mapx*mapy];
	//unsigned char* whereis=new unsigned char[mapx*mapy*4];
	//memset(whereis,0,mapx*mapy*4);
	//CBitmap wh("Geovent.bmp");
	//CBitmap whereMap=wh.CreateRescaled(mapx, mapy);
	//whereMap.Save("WhereIsItb4.bmp");
	float sub=minHeight;
	float mul=(1.0f/(maxHeight-minHeight))*0xffff;
	//whereSit=(whereSit-sub)*mul;
	//cout << "Looking for " << (unsigned short)whereSit;
	for(int y=0;y<mapy;++y){
		for(int x=0;x<mapx;++x){
			hm[y*mapx+x]=(unsigned short)((heightmap[y*mapx+x]-sub)*mul);
	/*		if (((hm[y*mapx+x-1]-whereSit<0) && (hm[y*mapx+x]-whereSit>0)) || ((hm[y*mapx+x-1]-whereSit)>0 && (hm[y*mapx+x]-whereSit<0) || (hm[y*mapx+x-1]-whereSit)==0))
			{
				unsigned char roy=whereMap.mem[(y*mapx+x)*4];
				whereMap.mem[(y*mapx+x)*4]=(unsigned char)255;
				unsigned char gee=whereMap.mem[(y*mapx+x)*4+1];
				whereMap.mem[(y*mapx+x)*4+1]=(unsigned char)255;
				unsigned char biv=whereMap.mem[(y*mapx+x)*4+2];
				whereMap.mem[(y*mapx+x)*4+2]=(unsigned char)255;
				//whereMap.mem[(y*mapx+x)*4+1]=(unsigned char)whereMap.mem[(y*mapx+x+1)]^(unsigned char)255;
				//whereMap.mem[(y*mapx+x)*4+2]=(unsigned char)whereMap.mem[(y*mapx+x+2)]^(unsigned char)255;
			}
			else
			{
				whereMap.mem[(y*mapx+x)*4]=(unsigned char)0;
				whereMap.mem[(y*mapx+x)*4+1]=(unsigned char)0;
				whereMap.mem[(y*mapx+x)*4+2]=(unsigned char)0;
			}
			/*else if ((hm[y*mapx+x]<(unsigned short)whereSit))
				whereis[(y*mapx+x)*4+1]=high;
			
			//	whereis[(y*mapx+x)*4+2]=255;*/
		}
	}
	outfile.write((char*)hm,mapx*mapy*2);

	//whereMap.mem=wheremap.mem^whereis;
	//whereMap.Save("WhereIsIt.bmp");
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

