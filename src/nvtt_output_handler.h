#ifndef __NVTT_OUTPUTHANDLER_H
#define __NVTT_OUTPUTHANDLER_H
#include <vector>
#include <nvtt/nvtt.h>

using namespace std;

class NVTTOutputHandler: public nvtt::OutputHandler
{
public:
	NVTTOutputHandler();
	NVTTOutputHandler(int buffer_size);
	~NVTTOutputHandler();

	void beginImage( int size, int width, int height, int depth, int face,
				int miplevel);

	bool writeData(const void *data, int size);

	void reset();

	struct mipinfo {
		int offset;
		int size;
		int width, height, depth;
		int face;
	};

	vector<mipinfo> mip;

	char *buffer;
	int buffer_size;
	int offset;
};

NVTTOutputHandler::NVTTOutputHandler()
{
	offset = 0;
}

NVTTOutputHandler::NVTTOutputHandler(int buffer_size) : buffer_size(buffer_size)
{
	buffer = new char[buffer_size];
	offset = 0;
}

NVTTOutputHandler::~NVTTOutputHandler()
{
	delete [] buffer;
}

void
NVTTOutputHandler::beginImage( int size, int width, int height, int depth, int face,
				int miplevel)
{
	mipinfo info = {offset, size, width, height, depth, face};
	mip.push_back(info);
	return;
}

bool
NVTTOutputHandler::writeData(const void *data, int size)
{
    // Copy mipmap data
	if(offset + size < buffer_size) {
	    memcpy( &buffer[offset], data, size );
    	offset += size;
	}
    return true;
}

void
NVTTOutputHandler::reset()
{
	offset = 0;
	mip.clear();
}


#endif //ndef __NVTT_OUTPUTHANDLER_H
