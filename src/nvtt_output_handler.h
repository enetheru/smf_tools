#ifndef __NVTT_OUTPUTHANDLER_H
#define __NVTT_OUTPUTHANDLER_H

#include <nvtt/nvtt.h>

class NVTTOutputHandler: public nvtt::OutputHandler
{
    unsigned int size, offset = 0;
    char *buffer;

public:
    NVTTOutputHandler( unsigned int size );
    ~NVTTOutputHandler( ){ delete [] buffer; };

    void beginImage( int, int, int, int, int, int ){ return; };
    bool writeData( const void *data, int size );

    void reset( unsigned int size);
    void reset( ){ offset = 0; };
    char *getBuffer( ){ return buffer; };
};

#endif //ndef __NVTT_OUTPUTHANDLER_H
