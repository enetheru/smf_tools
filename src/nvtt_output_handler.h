#ifndef __NVTT_OUTPUTHANDLER_H
#define __NVTT_OUTPUTHANDLER_H

#include <nvtt/nvtt.h>

class NVTTOutputHandler: public nvtt::OutputHandler
{
    int   buffer_size,
          offset = 0;
    char *buffer;

public:
    NVTTOutputHandler( int b ): buffer_size(b) { buffer = new char[ b ]; };
    ~NVTTOutputHandler     ( )                 { delete [] buffer;       };

    void beginImage(
            int size,
            int width,
            int height,
            int depth,
            int face,
            int miplevel )
        { return; };

    void reset     () { offset = 0; };
    char *getBuffer() {return buffer; };

    bool writeData( const void *data, int size );

};

#endif //ndef __NVTT_OUTPUTHANDLER_H
