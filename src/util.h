#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <vector>

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebufalgo.h>
OIIO_NAMESPACE_USING

void valxval( std::string s, unsigned int &x, unsigned int &y );

std::vector< unsigned int > expandString( const char *s );

/// Convert integer to hex string.
/*  Takes an integer and outputs its hex value as a string with leading '0x'
 */
template< typename T > std::string int_to_hex( T i ){
  std::stringstream stream;
  stream << "0x" 
         << std::setfill( '0' ) << std::setw( sizeof( T ) * 2 ) 
         << std::hex << i;
  return stream.str();
}

ImageBuf *channels( ImageBuf *sourceBuf, ImageSpec spec );

ImageBuf *scale( ImageBuf *sourceBuf, ImageSpec spec );
   
#endif //UTIL_H
