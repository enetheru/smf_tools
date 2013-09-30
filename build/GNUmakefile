CXXFLAGS=-g -Wall -L/usr/lib
TCLAP=-I../include/tclap-1.2.1/include
BOOSTLIBS=-lboost_system-mt -lboost_filesystem-mt -lboost_regex-mt

all: mapconv 

clean:
	rm -f mapconv *.o
	rm -f *~

mapconv: Bitmap.o MapConv.o TileHandler.o FeatureCreator.o FileHandler.o
	g++ $(CXXFLAGS) $^ -o $@ -lIL $(BOOSTLIBS) -lnvtt -lOpenImageIO

MapConv.o: MapConv.cpp Bitmap.h FileHandler.h
	g++ $(CXXFLAGS) -c $< $(TCLAP)

TileHandler.o: TileHandler.cpp TileHandler.h Bitmap.h FileHandler.h
	g++ $(CXXFLAGS) -c $<

FeatureCreator.o: FeatureCreator.cpp FeatureCreator.h Bitmap.h
	g++ $(CXXFLAGS) -c $<

Bitmap.o: Bitmap.cpp Bitmap.h FileHandler.h
	g++ $(CXXFLAGS) -c $<

FileHandler.o: FileHandler.cpp FileHandler.h
	g++ $(CXXFLAGS) -c $<
