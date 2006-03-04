#include "StdAfx.h"
#include ".\featurecreator.h"

#define NUM_TREE_TYPES 16
#define NUM_GRASS_TYPES 1

extern float* heightmap;

CFeatureCreator::CFeatureCreator(void)
{
}

CFeatureCreator::~CFeatureCreator(void)
{
}

void CFeatureCreator::WriteToFile(ofstream* file)
{
	FeatureHeader fh;
	fh.numFeatures=(int)features.size();
	fh.numFeatureType=NUM_TREE_TYPES+NUM_GRASS_TYPES+1;

	printf("Writing %i features\n",fh.numFeatures);

	file->write((char*)&fh,sizeof(fh));

	for(int a=0;a<NUM_TREE_TYPES;++a){
		char c[100];
		sprintf(c,"TreeType%i",a);
		file->write(c,(int)strlen(c)+1);
	}
	for(int a=0;a<NUM_GRASS_TYPES;++a){
		char c[100];
		sprintf(c,"GrassType%i",a);
		file->write(c,(int)strlen(c)+1);
	}
	char c[100];
	sprintf(c,"GeoVent",a);
	file->write(c,(int)strlen(c)+1);

	for(vector<FeatureFileStruct>::iterator fi=features.begin();fi!=features.end();++fi){
		file->write((char*)&*fi,sizeof(FeatureFileStruct));
	}
}

void CFeatureCreator::CreateFeatures(CBitmap* bm, int startx, int starty,std::string metalfile)
{
	printf("Creating features\n");
	int xsize=bm->xsize/8;
	int ysize=bm->ysize/8;
	int mapx=xsize+1;

	//geovents
	CBitmap vent("geovent.bmp");
	CBitmap metal(metalfile);		//use the green channel for geos

	for(int y=0;y<metal.ysize;++y){
		for(int x=0;x<metal.xsize;++x){
			unsigned char c=metal.mem[(y*metal.xsize+x)*4+1];
			if(c==255){
				int bx=x*xsize/metal.xsize;
				int by=y*ysize/metal.ysize;
				for(int tries=0;tries<1000;++tries){
					int x=bx+rand()*(40)/RAND_MAX-20;
					int y=by+rand()*(40)/RAND_MAX-20;
					if(x<5)
						x=5;
					if(x>xsize-5)
						x=xsize-5;
					if(y<5)
						y=5;
					if(y>ysize-5)
						y=ysize-5;

					float h=heightmap[y*mapx+x];
					if(h<5)
						continue;

					bool good=true;
					for(int y2=y-3;y2<=y+3;++y2){
						for(int x2=x-3;x2<=x+3;++x2){
							if(fabs(h-heightmap[(y2)*mapx+x2])>3)
								good=false;
						}
					}
					if(good){
						FeatureFileStruct ffs;
						ffs.featureType=NUM_TREE_TYPES+NUM_GRASS_TYPES;
						ffs.relativeSize=1;
						ffs.rotation=0;
						ffs.xpos=(float)x*8+4;
						ffs.ypos=0;
						ffs.zpos=(float)y*8+4;

						features.push_back(ffs);


						for(int y2=0;y2<vent.ysize;++y2){
							for(int x2=0;x2<vent.xsize;++x2){
								if(vent.mem[(y2*vent.xsize+x2)*4+0]!=255 || vent.mem[(y2*vent.xsize+x2)*4+2]!=255){
									bm->mem[((y*8+y2-vent.ysize/2)*bm->xsize+x*8+x2-vent.xsize/2)*4+0]=vent.mem[(y2*vent.xsize+x2)*4+0];
									bm->mem[((y*8+y2-vent.ysize/2)*bm->xsize+x*8+x2-vent.xsize/2)*4+1]=vent.mem[(y2*vent.xsize+x2)*4+1];
									bm->mem[((y*8+y2-vent.ysize/2)*bm->xsize+x*8+x2-vent.xsize/2)*4+2]=vent.mem[(y2*vent.xsize+x2)*4+2];
								}
							}
						}
						break;
					}
				}
			}
		}
	}

	//trees
	unsigned char* map=new unsigned char[ysize*xsize];
	for(int y=0;y<ysize;++y){
		for(int x=0;x<xsize;++x){
			map[y*xsize+x]=0;
		}
	}
	for(int y=0;y<ysize;++y){
		for(int x=0;x<xsize;++x){
			int red=0;
			int green=0;
			int blue=0;
			for(int y2=0;y2<8;++y2){
				for(int x2=0;x2<8;++x2){
					red+=bm->mem[((y*8+y2)*bm->xsize+x*8+x2)*4+0];
					green+=bm->mem[((y*8+y2)*bm->xsize+x*8+x2)*4+1];
					blue+=bm->mem[((y*8+y2)*bm->xsize+x*8+x2)*4+2];
				}
			}
			if(green>red*1.25 && green>blue*1.25){
				bool foundClose=false;
				for(int y2=max(0,y-3);y2<=y;++y2){
					for(int x2=max(0,x-3);x2<=min(xsize-1,x+3);++x2){
						if(map[y2*xsize+x2]){
							foundClose=true;
						}
					}
				}
				if(!foundClose && float(rand())/RAND_MAX>0.8){
					map[y*xsize+x]=1;
					FeatureFileStruct ffs;
					ffs.featureType=(rand()*NUM_TREE_TYPES)/RAND_MAX;
					ffs.relativeSize=0.8f+float(rand())/RAND_MAX*0.4f;
					ffs.rotation=0;
					ffs.xpos=(float)startx+x*8+4;
					ffs.ypos=0;
					ffs.zpos=(float)starty+y*8+4;

					features.push_back(ffs);
				}
			}
		}
	}
	//grass
	for(int y=0;y<ysize/4;++y){
		for(int x=0;x<xsize/4;++x){
			int red=0;
			int green=0;
			int blue=0;
			for(int y2=0;y2<32;++y2){
				for(int x2=0;x2<32;++x2){
					red+=bm->mem[((y*32+y2)*bm->xsize+x*32+x2)*4+0];
					green+=bm->mem[((y*32+y2)*bm->xsize+x*32+x2)*4+1];
					blue+=bm->mem[((y*32+y2)*bm->xsize+x*32+x2)*4+2];
				}
			}
			if(green>red*1.07 && green>blue*1.07 && heightmap[(y*4+2)*mapx+x*4+2]>2){
				map[y*xsize+x]=1;
				FeatureFileStruct ffs;
				ffs.featureType=NUM_TREE_TYPES;
				ffs.relativeSize=0;
				ffs.rotation=0;
				ffs.xpos=(float)x*32+16;
				ffs.ypos=0;
				ffs.zpos=(float)y*32+16;

				features.push_back(ffs);
			}
		}
	}
}
