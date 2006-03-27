#include "FeatureCreator.h"
#include "Bitmap.h"
//del later
#include <iomanip>
#define NUM_TREE_TYPES 16
#define treetop 215
extern float* heightmap;

#include <iostream> /* cout */
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
	//F_map=&F_map[1];

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
		//char c[100];
		//sprintf(c,"%s",F_map->a);
		//cout<< *F_map_iter;
		string c=*F_map_iter;
		file->write(c.c_str(),(int)strlen(c.c_str())+1);
	}

	for(vector<MapFeatureStruct>::iterator fi=features.begin();fi!=features.end();++fi){
		file->write((char*)&*fi,sizeof(MapFeatureStruct));
	}
}

void CFeatureCreator::CreateFeatures(CBitmap* bm, int startx, int starty,std::string metalfile, int arbFeatureTypes, std::string featurefile)
{
	printf("Creating features\n");
	xsize=bm->xsize/8;
	ysize=bm->ysize/8;
	int mapx=xsize+1;

	//geovents
	CBitmap vent("geovent.bmp");
//	CBitmap metal(metalfile);		//use the green channel for geos
	CBitmap feature(featurefile);
	unsigned char* map=new unsigned char[ysize*xsize];

	int LastTree[2]={0,0};
	for(int y=0;y<feature.ysize;++y){
		for(int x=0;x<feature.xsize;++x){
			unsigned char c=feature.mem[(y*feature.xsize+x)*4+1];
			unsigned char f=feature.mem[(y*feature.xsize+x)*4];
			//if (f) {cout << (int)f;}//Blue channel for arb features
			//cout<<x<<":"<<y<<" c="<<(int)c<<" f="<<(int)f<<"\n";
			if(c==255){
				cout<<"Geo at:";
				int bx=x*xsize/feature.xsize;
				int by=y*ysize/feature.ysize;
				for(int tries=0;tries<1000;++tries){
					/*int x=bx+rand()*(40)/RAND_MAX-20;
					int y=by+rand()*(40)/RAND_MAX-20;
					if(x<5)
						x=5;
					if(x>xsize-5)
						x=xsize-5;
					if(y<5)
						y=5;
					if(y>ysize-5)
						y=ysize-5;
*/
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
						//if(c==255){
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
					else {
#ifdef WIN32
						cout<< x <<":"<< y <<" X \009";
#else
						cout<< x <<":"<< y <<" X @";
#endif
#ifdef WIN32
						x=bx+rand()*(40)/RAND_MAX-20;
						y=by+rand()*(40)/RAND_MAX-20;
#else
						x=bx+( (float)(rand())*(40.0) )/RAND_MAX-20;
						y=by+( (float)(rand())*(40.0) )/RAND_MAX-20;
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
						//trees metalmap green 200-216
					if(c>199 && c<=treetop){
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
						if (!((LastTree[0]==(x-1) && LastTree[1]==y) || (LastTree[0]==x && LastTree[1]==(y-1)))){
								int t_type=(c-200);
								map[y*xsize+x]=1;
								MapFeatureStruct ffs;
								ffs.featureType=t_type;
								ffs.relativeSize=0.8f+float(rand())/RAND_MAX*0.4f;
								ffs.rotation=0;
								ffs.xpos=(float)startx+x*8+4;
								ffs.ypos=0;
								ffs.zpos=(float)starty+y*8+4;
								LastTree[0]=x;
								LastTree[1]=y;	
								features.push_back(ffs);
							}
						}
						//break;
		//}
						//Grass metalmap green 128 & not under water
						//if(c==128){
						//else{	vegMap[y*xsize+x]=1;
						//break;
					//break;
		
	
			
			if ((f) && ((256-f)<= arbFeatureTypes)) {
				cout<<"Feature Type"<<(256-f)<<" at:"<<x<<":"<<y<<endl;
				map[y*xsize+x]=1;
				MapFeatureStruct ffs;
				ffs.featureType=NUM_TREE_TYPES+(int)(256-f);
				ffs.relativeSize=1;
				ffs.rotation=0;
				ffs.xpos=(float)startx+x*8+4;
				ffs.ypos=0;
				ffs.zpos=(float)starty+y*8+4;
				features.push_back(ffs);
			//}
							
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
			unsigned char c=vegfeature.mem[(y*vegfeature.xsize+x)*4+2];
//			if (c==128){
//				vegMap[y*vegfeature.xsize+x]=1;
			float a=c/255;
			float b=1-a;
			int grass=(rand()%(255))+c;
			//float grass=c+rf;
			//cout << setprecision(3)<< fixed << grass<<" ";
			if (grass > 254)
			{
#ifdef WIN32
				cout<<"\002";
#else
				cout<<"@";
#endif
				vegMap[y*vegfeature.xsize+x]=1;
			}
			else
				cout<<" ";
				
		}
		cout<<"\n";
	}
	}

