#include <cstring>
#include "nvtt_output_handler.h"

using namespace std;

NVTTOutputHandler::NVTTOutputHandler( unsigned int size )
    : size(size)
{
    buffer = new char[ size ];
}

bool
NVTTOutputHandler::writeData( const void *data, int size )
{
    // Copy mipmap data
    if( offset + size <= this->size ) {
        memcpy( buffer + offset, data, size );
        offset += size;
    }
    return true;
}

void
NVTTOutputHandler::reset( unsigned int size )
{
    offset = 0;
    delete [] buffer;
    buffer = new char[ size ];
}
