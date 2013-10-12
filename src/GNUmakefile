CXXFLAGS=-g -Wall -L/usr/lib
TCLAP=-I../include/tclap-1.2.1/include

all: mapconv tileconv

clean:
	rm -f mapconv *.o
	rm -f *~

mapconv: main.o 
	g++ $(CXXFLAGS) $^ -o $@ -lnvtt -lOpenImageIO

main.o: main.cpp mapfile.h tools.h
	g++ $(CXXFLAGS) -c $< $(TCLAP)

tileconv: tileconv.o 
	g++ $(CXXFLAGS) $^ -o $@ -lnvtt -lOpenImageIO

tileconv.o: tileconv.cpp tools.h nvtt_output_handler.h smt.h
	g++ $(CXXFLAGS) -c $< $(TCLAP)
