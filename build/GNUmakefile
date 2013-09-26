CXXFLAGS=-g -Wall -L/usr/lib
SDLCFLAGS=$(SDLFLAGS) $(shell sdl-config --cflags)
SDLLIBS=$(shell sdl-config --libs)
TCLAP=-I../include/tclap-1.0.5/include


all: mapconv texcompress_nogui texcompress

clean:
	rm -f mapconv texcompress texcompress_nogui *.o
	rm -f *~

mapconv: Bitmap.o MapConv.o TileHandler.o FeatureCreator.o FileHandler.o
	g++ $(CXXFLAGS) $^ -o $@ -lIL -lboost_regex-mt -lboost_filesystem-mt -lboost_system-mt

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

texcompress.o: texcompress.cpp
	g++ $(CXXFLAGS) -c $^ -o $@ $(SDLCFLAGS)

texcompress: texcompress.o Bitmap.o FileHandler.o
	g++ $(CXXFLAGS) $^ -o $@ $(SDLLIBS) -lboost_filesystem-mt -lboost_regex-mt -lboost_system -lGLU -lGLEW -lIL

texcompress_nogui.o: texcompress_nogui.cpp
	g++ $(CXXFLAGS) -c $^ -o $@
texcompress_nogui: texcompress_nogui.o Bitmap.o FileHandler.o
	g++ $(CXXFLAGS) $^ -o $@ -ldl -lboost_filesystem-mt -lboost_regex-mt -lboost_system -lIL

