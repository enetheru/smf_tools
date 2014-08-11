#ifndef UTIL_H
#define UTIL_H

#include <OpenImageIO/imagebuf.h>
#include <cstdint>
#include <string>
#include <vector>
#include <iomanip>

// Evaluates a string into two integers
/* The function takes a string in the form of IxK, and assignes
 * the values of I,K to x,y.
 */
void valxval( std::string s, uint32_t &x, uint32_t &y );

/// Expand a string sequence of integers to a vector
/*  The function takes a string in the form of comma ',' separated values.
 *  When two values are separated with a dash '-' then all the numbers
 *  inbetween are expanded.\n
 *  ie '1,2,3-7,2-5' = {1,2,3,4,5,6,7,2,3,4,5}
 */
std::vector< uint32_t > expandString( const char *s );

/// Convert integer to hex string.
/*  Takes an integer and outputs its hex value as a string with leading '0x'
 */
template< typename T >
std::string int_to_hex( T i )
{
  std::stringstream stream;
  stream << "0x" 
         << std::setfill( '0' ) << std::setw( sizeof( T ) * 2 ) 
         << std::hex << i;
  return stream.str();
}

/// re-orders the channels an imageBuf according to the imagespec
/*  Returns a copy of the souceBuf with the number of channels in
 *  the ImageSpec spec. if there is no Alpha than a opaque one is created.
 *  If sourceBuf is NULL then a blank image is returned.
 */
OpenImageIO::ImageBuf *channels( OpenImageIO::ImageBuf *sourceBuf,
        OpenImageIO::ImageSpec spec );

/// Scales an ImageBuf according to a given ImageSpec
/*  If sourceBuf is NULL then return a blank image.
 */
OpenImageIO::ImageBuf *scale( OpenImageIO::ImageBuf *sourceBuf,
        OpenImageIO::ImageSpec spec );
   
#endif //UTIL_H
