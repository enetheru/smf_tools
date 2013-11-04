#ifndef __NVTT_OUTPUTHANDLER_H
#define __NVTT_OUTPUTHANDLER_H

#include <nvtt/nvtt.h>

class NVTTOutputHandler: public nvtt::OutputHandler
{
public:
	NVTTOutputHandler(int buffer_size);
	~NVTTOutputHandler();

	void beginImage( int size, int width, int height, int depth, int face, int miplevel);
	bool writeData(const void *data, int size);
	void reset();

	char *buffer;
	int buffer_size;
	int offset;
};

#endif //ndef __NVTT_OUTPUTHANDLER_H
