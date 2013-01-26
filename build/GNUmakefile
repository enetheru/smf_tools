CXXFLAGS=-g -Wall -L/usr/lib
SDLCFLAGS=$(SDLFLAGS) $(shell sdl-config --cflags)
SDLLIBS=$(shell sdl-config --libs)


all: mapconv texcompress_nogui texcompress

clean:
	rm -f mapconv texcompress texcompress_nogui *.o
	rm -f *~

texcompress: texcompress.o Bitmap.o FileHandler.o
	g++ $(CXXFLAGS) $(SDLLIBS) -lboost_filesystem-mt -lboost_regex-mt -lGLU -lGLEW -lIL  $^ -o $@

texcompress.o: texcompress.cpp
	g++ $(CXXFLAGS) $(SDLCFLAGS) -c $^ -o $@

mapconv: Bitmap.o MapConv.o TileHandler.o FeatureCreator.o FileHandler.o
	g++ $(CXXFLAGS) -lIL -lboost_regex-mt -lboost_filesystem-mt $^ -o $@

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


# nogui stuff
texcompress_nogui.o: texcompress_nogui.cpp
	g++ $(CXXFLAGS) -c $^ -o $@
texcompress_nogui: texcompress_nogui.o Bitmap.o FileHandler.o
	g++ $(CXXFLAGS) -ldl -lboost_filesystem-mt -lboost_regex-mt -lIL $^ -o $@

