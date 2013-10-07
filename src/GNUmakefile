CXXFLAGS=-g -Wall -L/usr/lib
TCLAP=-I../include/tclap-1.2.1/include

all: mapconv #tileconv

clean:
	rm -f mapconv *.o
	rm -f *~

mapconv: main.o 
	g++ $(CXXFLAGS) $^ -o $@ -lnvtt -lOpenImageIO

main.o: main.cpp 
	g++ $(CXXFLAGS) -c $< $(TCLAP)

#tileconv: tileconv.o
#	g++ $(CXXFLAGS) $^ -o $@ -lIL $(BOOSTLIBS) -lnvtt -lOpenImageIO

#tileconv.o: tileconv.cpp
#	g++ $(CXXFLAGS) -c $< $(TCLAP)
