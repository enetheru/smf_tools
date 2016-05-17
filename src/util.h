#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <iomanip>
#include <elog.h>

#include <OpenImageIO/imagebuf.h>

// Evaluates a string into two integers
/* The function takes a string in the form of IxK, and assignes
 * the values of I,K to x,y.
 */
std::pair< uint32_t, uint32_t > valxval( const std::string s );

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
std::string to_hex(T i)
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
 *  If sourceBuf is nullptr then a blank image is returned.
 */
std::unique_ptr< OpenImageIO::ImageBuf > fix_channels(
    std::unique_ptr< OpenImageIO::ImageBuf> &&,
    const OpenImageIO::ImageSpec & );

void channels( OpenImageIO::ImageBuf *&sourceBuf,
        OpenImageIO::ImageSpec spec );

/// Scales an ImageBuf according to a given ImageSpec
/* in place scale */
std::unique_ptr< OpenImageIO::ImageBuf > fix_scale(
    std::unique_ptr< OpenImageIO::ImageBuf> &&,
    const OpenImageIO::ImageSpec & );

void scale( OpenImageIO::ImageBuf *&sourceBuf,
        OpenImageIO::ImageSpec spec );

/// output a progress indicator
void progressBar( std::string message, float goal, float progress );

/// simple map class for checking overlapping regions of memory
/*
 *
 */
//FIXME rename this and move to own source file
class FileMap{
    struct Block{
        uint32_t begin;
        uint32_t end;
        std::string name;
    };

    std::vector< Block > list;

    public:
    void addBlock( uint32_t begin, uint32_t size, std::string name = "")
    {
        DLOG( INFO ) << "adding: " << name << "(" << begin << "-" << begin + size-1 << ")";
        Block temp{ begin, begin + size-1, name };
        for( auto i : list ){
            if( (temp.begin >= i.begin && temp.begin <= i.end)
             || (temp.end >= i.begin && temp.end <= i.end)
             || (temp.begin <= i.begin && temp.end >= i.end) ){
                LOG( ERROR ) << "'" << temp.name << "'"
                    << " clashes with existing block " << "'" << i.name << "'";
            }
        }
        list.push_back( temp );
    }

};

// Convert an image to ascii so we can get direct feedback from the app
/* FIXME does this need to use some sort of enumerator for  the type? probably
 * infact homogenising the image types used in the lib is probably a good idea.
 *
 */
std::string
image_to_hex( const uint8_t *data, int width, int height, int type = 0 );
