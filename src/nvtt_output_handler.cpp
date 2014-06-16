#include <cstring>
#include "nvtt_output_handler.h"

using namespace std;

bool
NVTTOutputHandler::writeData( const void *data, int size )
{
    // Copy mipmap data
    if( offset + size <= buffer_size ) {
        memcpy( &buffer[ offset ], data, size );
        offset += size;
    }
    return true;
}
