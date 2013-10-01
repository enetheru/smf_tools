CXXFLAGS=-g -Wall -L/usr/lib
TCLAP=-I../include/tclap-1.2.1/include
BOOSTLIBS=-lboost_system-mt -lboost_filesystem-mt -lboost_regex-mt

all: mapconv 

clean:
	rm -f mapconv *.o
	rm -f *~

mapconv: bitmap.o main.o tilehandler.o featurecreator.o filehandler.o
	g++ $(CXXFLAGS) $^ -o $@ -lIL $(BOOSTLIBS) -lnvtt -lOpenImageIO

main.o: main.cpp bitmap.h filehandler.h
	g++ $(CXXFLAGS) -c $< $(TCLAP)

tilehandler.o: tilehandler.cpp tilehandler.h bitmap.h filehandler.h
	g++ $(CXXFLAGS) -c $<

featurecreator.o: featurecreator.cpp featurecreator.h bitmap.h
	g++ $(CXXFLAGS) -c $<

bitmap.o: bitmap.cpp bitmap.h filehandler.h
	g++ $(CXXFLAGS) -c $<

filehandler.o: filehandler.cpp filehandler.h
	g++ $(CXXFLAGS) -c $<
