#include "StdAfx.h"
#include ".\tilehandler.h"
#include "filehandler.h"
#include "ddraw.h"
#include "sm2header.h"

CTileHandler tileHandler;

CTileHandler::CTileHandler(void)
{
}

CTileHandler::~CTileHandler(void)
{
	for(int a=0;a<usedTiles;++a)
		delete tiles[a];
}

void CTileHandler::LoadTexture(string name)
{
	printf("Loading texture\n");
	bigTex.Load(name);

	xsize=bigTex.xsize/8;
	ysize=bigTex.ysize/8;
}

void CTileHandler::ProcessTiles(float compressFactor)
{
	meanThreshold=(int)(2000*compressFactor);
	meanDirThreshold=(int)(20000*compressFactor);
	borderThreshold=(int)(80000*compressFactor);

	system("mkdir temp");

	usedTiles=0;
	for(int y=0; y<ysize/4; y++){
		for(int x=0; x<xsize/4; x++){
			CBitmap* bm=new CBitmap(bigTex.GetRegion(x*32, y*32, 32, 32));
			int t1=tileUse[max(0,(y-1)*xsize/4+x)];
			int t2=tileUse[max(0,y*xsize/4+x-1)];
			int ct=FindCloseTile(bm,t1==t2?t1:-1);
			if(ct==-1){
				tileUse[y*xsize/4+x]=usedTiles;
				tiles[usedTiles++]=bm;
			} else {
				tileUse[y*xsize/4+x]=ct;
			}
		}
		printf("Creating tiles %i/%i %i%%\n", usedTiles,y*xsize/4+x,((y*xsize/4+x)*100)/(xsize/4*ysize/4));
	}
	
}

void CTileHandler::ProcessTiles2(void)
{
	unsigned char* data=new unsigned char[1024*1024*4];
	bigTex=CBitmap(data,1,1);	//free big tex memory
	
	char execstring[512];
	sprintf(execstring, "nvdxt.exe -file temp\\*.bmp -dxt1c -dither");

	for(int a=0;a<(usedTiles+1023)/1024;++a){
		int startTile=a*1024;
		for(int b=0;b<1024 && startTile+b<usedTiles;++b){
			int x=b%32;
			int y=b/32;
			for(int y2=0;y2<32;++y2){
				for(int x2=0;x2<32;++x2){
					data[((y*32+y2)*1024+x*32+x2)*4+0]=tiles[startTile+b]->mem[((y2)*32+x2)*4+0];
					data[((y*32+y2)*1024+x*32+x2)*4+1]=tiles[startTile+b]->mem[((y2)*32+x2)*4+1];
					data[((y*32+y2)*1024+x*32+x2)*4+2]=tiles[startTile+b]->mem[((y2)*32+x2)*4+2];
					data[((y*32+y2)*1024+x*32+x2)*4+3]=tiles[startTile+b]->mem[((y2)*32+x2)*4+3];
				}
			}
		}
		CBitmap square(data,1024,1024);
		char name[100];
		sprintf(name,"temp\\Temp%03i.bmp",a);
		square.Save(name);
		printf("Writing bmp files %i%%\n", ((a*1024)*100)/(usedTiles));
	}
	printf("Creating dds files\n");
	system(execstring);
	system("del temp\\temp*.bmp");

	delete[] data;
}

void CTileHandler::SaveData(ofstream& ofs)
{
	for(int y=0; y<ysize/4; y++)
	{
		for(int x=0; x<xsize/4; x++)
		{
			int num = tileUse[x+y*xsize/4];
			ofs.write((char*)&num, sizeof(int));
		}
	}

	for(int a=0;a<(usedTiles+1023)/1024;++a){
		int startTile=a*1024;
		DDSURFACEDESC2 ddsheader;
		int ddssignature;

		char name[100];
		sprintf(name,"Temp%03i.dds",a);
		CFileHandler file(name);
		file.Read(&ddssignature, sizeof(int));
		file.Read(&ddsheader, sizeof(DDSURFACEDESC2));

		char bigtile[696320]; //1024x1024 and 4 mipmaps
		file.Read(bigtile, 696320);

		for(int b=0;b<1024 && startTile+b<usedTiles;++b){
			int x=b%32;
			int y=b/32;
			char tile[SMALL_TILE_SIZE];
			ReadTile(x*32,y*32,tile,bigtile);
			ofs.write(tile,SMALL_TILE_SIZE);
		}
		printf("Writing tiles %i%%\n", ((a*1024)*100)/(usedTiles));
	}

	system("del temp*.dds");
}

void CTileHandler::ReadTile(int xpos, int ypos, char *destbuf, char *sourcebuf)
{
	int doffset = 0;
	int soffset = 0;

	for(int i=0; i<4; i++)
	{
		int div = 1<<i;
		int xp = 8/div;
		int yp = 8/div;
		for(int y=0; y<yp; y++)
		{
			for(int x=0; x<xp; x++)
			{
				char *destptr = &destbuf[(x+y*xp)*8 + doffset];
				char *srcptr = &sourcebuf[((x+xpos/div/4)+((y+ypos/div/4))*(256/(div)))*8 + soffset];

				for(int i=0; i<8; i++)
					destptr[i] = srcptr[i];
			}
		}
		doffset += 512/(1<<(i*2));
		soffset += 524288/(1<<(i*2));
	}
}

int CTileHandler::FindCloseTile(CBitmap* bm,int forbidden)
{
	FastStat fs=CalcFastStat(bm);
	for(int a=0;a<usedTiles;++a){
		if(a!=forbidden &&
			 abs(fs.r-fastStats[a].r)<meanThreshold && abs(fs.g-fastStats[a].g)<meanThreshold && abs(fs.b-fastStats[a].b)<meanThreshold
		&& abs(fs.rx-fastStats[a].rx)<meanDirThreshold && abs(fs.gx-fastStats[a].gx)<meanDirThreshold && abs(fs.bx-fastStats[a].bx)<meanDirThreshold
		&& abs(fs.ry-fastStats[a].ry)<meanDirThreshold && abs(fs.gy-fastStats[a].gy)<meanDirThreshold && abs(fs.by-fastStats[a].by)<meanDirThreshold
		&& CompareTiles(bm,tiles[a]))
			return a;
	}
	fastStats[usedTiles]=fs;
	return -1;
}


CTileHandler::FastStat CTileHandler::CalcFastStat(CBitmap* bm)
{
	FastStat fs;
	fs.r=0;
	fs.g=0;
	fs.b=0;

	fs.rx=0;
	fs.gx=0;
	fs.bx=0;

	fs.ry=0;
	fs.gy=0;
	fs.by=0;

	for(int y=0;y<32;++y){
		for(int x=0;x<32;++x){
			fs.r+=bm->mem[(y*32+x)*4+0];
			fs.g+=bm->mem[(y*32+x)*4+1];
			fs.b+=bm->mem[(y*32+x)*4+2];

			fs.rx+=bm->mem[(y*32+x)*4+0]*(x-16);
			fs.gx+=bm->mem[(y*32+x)*4+1]*(x-16);
			fs.bx+=bm->mem[(y*32+x)*4+2]*(x-16);

			fs.ry+=bm->mem[(y*32+x)*4+0]*(y-16);
			fs.gy+=bm->mem[(y*32+x)*4+1]*(y-16);
			fs.by+=bm->mem[(y*32+x)*4+2]*(y-16);
		}
	}
	return fs;
}

bool CTileHandler::CompareTiles(CBitmap* bm, CBitmap* bm2)
{
	int totalerror=0;
	for(int y=0;y<32;++y){
		for(int x=0;x<32;++x){
			if(!(y==0 || y==31 || x==0 || x==31))
				continue;
			int rdif=bm->mem[(y*32+x)*4+0]-bm2->mem[(y*32+x)*4+0];
			int gdif=bm->mem[(y*32+x)*4+1]-bm2->mem[(y*32+x)*4+1];
			int bdif=bm->mem[(y*32+x)*4+2]-bm2->mem[(y*32+x)*4+2];
			int error=rdif*rdif+gdif*gdif+bdif*bdif;
			totalerror+=error;
		}
		if(totalerror>borderThreshold)
			return false;
	}
	return true;
}
