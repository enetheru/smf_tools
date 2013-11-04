#include <cstring>
#include "nvtt_output_handler.h"

using namespace std;

NVTTOutputHandler::NVTTOutputHandler(int buffer_size)
:	buffer_size(buffer_size),
	offset(0)
{
	buffer = new char[buffer_size];
}

NVTTOutputHandler::~NVTTOutputHandler()
{
	delete [] buffer;
}

void
NVTTOutputHandler::beginImage( int size, int width, int height, int depth,
		int face, int miplevel)
{
	return;
}

bool
NVTTOutputHandler::writeData(const void *data, int size)
{
    // Copy mipmap data
	if(offset + size <= buffer_size) {
	    memcpy( &buffer[offset], data, size );
    	offset += size;
	}
    return true;
}

void
NVTTOutputHandler::reset()
{
	offset = 0;
}
