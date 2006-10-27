CXXFLAGS=-g

all: mapconv texcompress

clean:
	rm -f mapconv texcompress *.o

texcompress: texcompress.o Bitmap.o FileHandler.o 
	g++ $(CXXFLAGS) -lboost_filesystem -lboost_regex -lIL -lGLEW -lGLU -lGL $(shell sdl-config --cflags) $(shell sdl-config --libs) $^ -o $@

texcompress.o: texcompress.cpp
	g++ $(CXXFLAGS) $(shell sdl-config --cflags) $(TEXCOMPRESS_INCLUDE) -c $^ -o $@

mapconv: Bitmap.o MapConv.o TileHandler.o FeatureCreator.o FileHandler.o
	g++ $(CXXFLAGS) -lIL -lboost_regex -lboost_filesystem $^ -o $@ 

MapConv.o: MapConv.cpp Bitmap.h FileHandler.h
	g++ $(CXXFLAGS) -c $< -Itclap-1.0.5/include/

TileHandler.o: TileHandler.cpp TileHandler.h Bitmap.h FileHandler.h
	g++ $(CXXFLAGS) -c $< 

FeatureCreator.o: FeatureCreator.cpp FeatureCreator.h Bitmap.h
	g++ $(CXXFLAGS) -c $< 

Bitmap.o: Bitmap.cpp Bitmap.h FileHandler.h
	g++ $(CXXFLAGS) -c $< 

FileHandler.o: FileHandler.cpp FileHandler.h
	g++ $(CXXFLAGS) -c $< 

