CXXFLAGS=-g -Wall -L/usr/lib
TCLAP=-I../include/tclap-1.2.1/include

all: mapconv tileconv

clean:
	rm -f mapconv *.o
	rm -f *~

mapconv: mapconv.o smt.o smf.o nvtt_output_handler.o dxt1load.o
	g++ $(CXXFLAGS) $^ -o $@ -lnvtt -lOpenImageIO -lboost_system

tileconv: tileconv.o smt.o smf.o nvtt_output_handler.o dxt1load.o
	g++ $(CXXFLAGS) $^ -o $@ -lnvtt -lOpenImageIO -lboost_system

dxt1load.o: dxt1load.cpp dxt1load.h 
	g++ $(CXXFLAGS) -c $< 

mapconv.o: mapconv.cpp dxt1load.h smf.h nvtt_output_handler.h
	g++ $(CXXFLAGS) -c $< $(TCLAP)

nvtt_output_handler.o: nvtt_output_handler.cpp nvtt_output_handler.h 
	g++ $(CXXFLAGS) -c $< 

smf.o: smf.cpp smf.h dxt1load.h nvtt_output_handler.h
	g++ $(CXXFLAGS) -c $< 

smt.o: smt.cpp smt.h dxt1load.h nvtt_output_handler.h
	g++ $(CXXFLAGS) -c $< 

tileconv.o: tileconv.cpp dxt1load.h nvtt_output_handler.h smt.h
	g++ $(CXXFLAGS) -c $< $(TCLAP)
