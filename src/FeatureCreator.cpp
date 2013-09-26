#include "FeatureCreator.h"
#include "Bitmap.h"
#include "math.h"
#ifdef WIN32
#include "stdafx.h"
#endif
#define NUM_TREE_TYPES 16
#define treetop 215 /* TODO Shouldn't this be related to the previous define somehow? */
extern float* heightmap;
extern short int* rotations;
extern int randomrotatefeatures;

#include <iostream> /* cout */
#include <string.h>
#include <stdlib.h>
using namespace std; /* cout */


CFeatureCreator::CFeatureCreator(void)
{
}

CFeatureCreator::~CFeatureCreator(void)
{
}

void CFeatureCreator::WriteToFile(ofstream* file, vector<string> F_map)
{
	//write vegetation map
	file->write((char*)vegMap,xsize/4*ysize/4);
	delete[] vegMap;

	vector<string>::iterator F_map_iter;
	int ArbFeatureTypes=F_map.size();

	//write features
	MapFeatureHeader fh;
	fh.numFeatures=(int)features.size();
	fh.numFeatureType=NUM_TREE_TYPES+1+ArbFeatureTypes;

	printf("Writing %i features\n",fh.numFeatures);

	file->write((char*)&fh,sizeof(fh));

	for(int a=0;a<NUM_TREE_TYPES;++a){
		char c[100];
		sprintf(c,"TreeType%i",a);
		file->write(c,(int)strlen(c)+1);
	}
	char c[100];
	sprintf(c,"GeoVent");
	file->write(c,(int)strlen(c)+1);

	for (F_map_iter=F_map.begin();F_map_iter!=F_map.end();F_map_iter++){
		string c=*F_map_iter;
		file->write(c.c_str(),(int)strlen(c.c_str())+1);
	}

	for(vector<MapFeatureStruct>::iterator fi=features.begin();fi!=features.end();++fi){
		file->write((char*)&*fi,sizeof(MapFeatureStruct));
	}
}

void CFeatureCreator::CreateFeatures(CBitmap* bm, int startx, int starty, int arbFeatureTypes, std::string featurefile, std::string geoVentFile, vector<LuaFeature> lf,vector<string> F_Spec)
{
	printf("Creating features\n");
	xsize=bm->xsize/8;
	ysize=bm->ysize/8;
	mapx=xsize+1;

	//geovent decal
	CBitmap vent(geoVentFile);
	CBitmap feature(featurefile);
	unsigned char* map=new unsigned char[ysize*xsize];
	for (unsigned int i=0; i<lf.size(); i++){

				MapFeatureStruct ffs;
				int ftype=-1; //now we must look up the lua feature types number identifer from the Luafeatures vector:
				for (unsigned int j=0; j<F_Spec.size();j++){
					if (F_Spec[j].compare(lf[i].name)==0){
						ftype=j;
					}
				}
				if (ftype==-1) {
					printf("feature not found in f_spec, %s \n", lf[i].name.c_str());
					continue;
				}

			
				ffs.featureType=NUM_TREE_TYPES+ftype+1;
				ffs.relativeSize=1;
				if (lf[i].rot==-1){//yep, we are still keeping it. -1 rotates randomly.
					ffs.rotation=(float(2*rand()-RAND_MAX));
				}else{
					ffs.rotation=lf[i].rot;
				}
				ffs.xpos=(float)startx+lf[i].posx;
				ffs.ypos=0;
				ffs.zpos=(float)starty+lf[i].posz;
				features.push_back(ffs);
				cout<<"Feature Type: "<<lf[i].name<<" at:"<<ffs.xpos<<":"<<ffs.zpos<<" rotation="<<ffs.rotation<<endl;
	}

	for(int y=0;y<feature.ysize;++y){
		for(int x=0;x<feature.xsize;++x){
			/* Read vents and trees from the green channel. */
			unsigned char c=feature.mem[(y*feature.xsize+x)*4+1];
			if(c==255){
				PlaceVent(x, y, &feature, &vent, bm);
			}
			else if(c>199 && c<=treetop){
				//trees featuremap green 200-215
				float h=heightmap[y*mapx+x];
				if(h<5)
					continue;

//				bool good=true;
//				if (!FlatSpot(x, y))
//					good = false;

					int t_type=(c-200);
					map[y*xsize+x]=1;
					MapFeatureStruct ffs;
					ffs.featureType=t_type;
					ffs.relativeSize=0.8f+float(rand())/RAND_MAX*0.4f;
					ffs.rotation=(float(2*rand()-RAND_MAX));
					ffs.xpos=(float)startx+x*8+4;
					ffs.ypos=0;
					ffs.zpos=(float)starty+y*8+4;
					features.push_back(ffs);
				//}
			} 
			else if (c != 0){
				printf("Does nothing: green %02x at X %d Y %d\n", c, x, y);
			}


			/* Read fs.txt's features from the red channel. */
			unsigned char f=feature.mem[(y*feature.xsize+x)*4];
			if ((f) && ((256-f)<= arbFeatureTypes)) {
				map[y*xsize+x]=1;
				MapFeatureStruct ffs;
				ffs.featureType=NUM_TREE_TYPES+(int)(256-f);
				ffs.relativeSize=1;
				if (f>=256- randomrotatefeatures || rotations[255-(int)f]==-1)
					ffs.rotation=(float(2*rand()-RAND_MAX));
				else 
					ffs.rotation=rotations[255-(int)f];
				
				ffs.xpos=(float)startx+x*8+4;
				ffs.ypos=0;
				ffs.zpos=(float)starty+y*8+4;
				features.push_back(ffs);
				cout<<"Feature Type"<<(256-f)<<" at:"<<x<<":"<<y<<" rr="<<randomrotatefeatures<<" val="<<ffs.rotation<<endl;
			}
			else if (f != 0){
				printf("Does nothing: red %02x at X %d Y %d (put more lines in fs.txt?)\n",
					f, x, y);
			}
		}
	}



	//grass take 2
	CBitmap vegfeature=feature.CreateRescaled(xsize/4,ysize/4);
	vegMap=new unsigned char[vegfeature.xsize*vegfeature.ysize];
	memset(vegMap,0,vegfeature.xsize*vegfeature.ysize);
	cout<<"Grass Placement:\n";
	for(int y=0;y<ysize/4;++y){
		for(int x=0;x<vegfeature.xsize;++x){
			/* Read grass from the blue channel. */
			unsigned char c=vegfeature.mem[(y*vegfeature.xsize+x)*4+2];
//			float a=c/255;
//			float b=1-a;
			int grass=(rand()%(255))+c;
			if (grass > 254)
			{
#ifdef WIN32
				cout<<"\002";
#else
				cout<<"@";
#endif
				vegMap[y*vegfeature.xsize+x]=1;
			}
			
				//cout<<" ";
		}
		cout<<"\n";
	}
}

void CFeatureCreator::PlaceVent(int x, int y, CBitmap * feature, CBitmap * vent, CBitmap * bm)
{
	cout<<"Geo at:";
	int bx=x*xsize/feature->xsize;
	int by=y*ysize/feature->ysize;
	for(int tries=0;tries<1000;++tries){
		float h=heightmap[y*mapx+x];
		if(h<5)
			return;

		bool good=true;
		
		if (!FlatSpot(x, y))
			good = false;
		
		if(good){
#ifdef WIN32
			cout<<""<<x<<":"<<y<<" \001\n";
#else
			cout<<""<<x<<":"<<y<<" *\n";
#endif
			MapFeatureStruct ffs;
			ffs.featureType=NUM_TREE_TYPES;
			ffs.relativeSize=1;
			ffs.rotation=0;
			ffs.xpos=(float)x*8+4;
			ffs.ypos=0;
			ffs.zpos=(float)y*8+4;

			features.push_back(ffs);

			// Draw the 'vent crack' on the map.
			for(int y2=0;y2<vent->ysize;++y2){
				for(int x2=0;x2<vent->xsize;++x2){
					if(vent->mem[(y2*vent->xsize+x2)*4+0]!=255 || vent->mem[(y2*vent->xsize+x2)*4+2]!=255){
						bm->mem[((y*8+y2-vent->ysize/2)*bm->xsize+x*8+x2-vent->xsize/2)*4+0]=vent->mem[(y2*vent->xsize+x2)*4+0];
						bm->mem[((y*8+y2-vent->ysize/2)*bm->xsize+x*8+x2-vent->xsize/2)*4+1]=vent->mem[(y2*vent->xsize+x2)*4+1];
						bm->mem[((y*8+y2-vent->ysize/2)*bm->xsize+x*8+x2-vent->xsize/2)*4+2]=vent->mem[(y2*vent->xsize+x2)*4+2];
					}
				}
			}
			break;
		}
		else {
			/* Roll up somewhere else to try and put it. */
#ifdef WIN32
			cout<< x <<":"<< y <<" X \009";
#else
			cout<< x <<":"<< y <<" X @";
#endif
#ifdef WIN32
			x=bx+rand()*(20)/RAND_MAX-10;
			y=by+rand()*(20)/RAND_MAX-10;
#else
			x=bx+( (float)(rand())*(20.0) )/RAND_MAX-10;
			y=by+( (float)(rand())*(20.0) )/RAND_MAX-10;
#endif
			if(x<5)
				x=5;
			if(x>xsize-5)
				x=xsize-5;
			if(y<5)
				y=5;
			if(y>ysize-5)
				y=ysize-5;
		}

	}
}

bool CFeatureCreator::FlatSpot(int x, int y)
{
	float h=heightmap[y*mapx+x];

	// Check that the ground around this spot is about the same elevation.
	for(int y2=y-3;y2<=y+3;++y2){

		// Don't check off the heightmap.
		if (y2 < 0)
			continue;
		if (y2 > ysize)
			continue;

		for(int x2=x-3;x2<=x+3;++x2){
		
			if (x2 < 0)
				continue;
			if (x2 > xsize)
				continue;
		
			if(fabs(h-heightmap[(y2)*mapx+x2])>20)
				return false;
		}
	}

	return true;
}

