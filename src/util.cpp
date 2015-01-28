#include <cstdint>
#include <string>
#include <vector>
#include <sstream>

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>

#include "elog/elog.h"

#include "util.h"

void
valxval( std::string s, uint32_t &x, uint32_t &y )
{
    uint32_t d;
    d = s.find_first_of( 'x', 0 );

    if(d) x = std::stoi( s.substr( 0, d ) );
    else x = 0;

    if(d == s.size()-1 ) y = 0;
    else y = std::stoi( s.substr( d + 1, std::string::npos) );
}

std::vector< uint32_t >
expandString( const char *s )
{
    std::vector< uint32_t > result;
    int start;
    bool sequence = false, fail = false;
    const char *begin;

    do {
        begin = s;

        while( *s != ',' && *s != '\n' && *s != '-' && *s ) s++;
        if( begin == s) continue;

        if( sequence ){
            for( int i = start; i < std::stoi( std::string( begin, s ) ); ++i )
                result.push_back( i );
        }

        if( *(s) == '-' ){
            sequence = true;
            try {
                start = std::stoi( std::string( begin, s ) );
            }
            catch( std::invalid_argument ){
                fail = true;
            }
        } else {
            sequence = false;
            try {
                result.push_back( std::stoi( std::string( begin, s ) ) );
            }
            catch( std::invalid_argument ){
                fail = true;
            }
        }
    }
    while( *s++ != '\0' );

    if( fail ){
        LOG( WARN ) << "Possible Parse Error while spliting filter string";
    }
    return result;
}

void
scale( OpenImageIO::ImageBuf *&sourceBuf, OpenImageIO::ImageSpec spec )
{
    OIIO_NAMESPACE_USING;

    CHECK( sourceBuf ) << " NULL pointer passed to scale()";

    // do nothing if the original if its the correct size.
    if( sourceBuf->spec().width == spec.width && sourceBuf->spec().height == spec.height ){
        return;
    }

    // Otherwise scale
    ROI roi(0, spec.width, 0, spec.height, 0, 1, 0, sourceBuf->spec().nchannels);
    ImageBuf *tempBuf = new ImageBuf;
    ImageBufAlgo::resize( *tempBuf, *sourceBuf, "", false, roi );
    sourceBuf->clear();
    delete sourceBuf;
    sourceBuf = tempBuf;
}

void
channels( OpenImageIO::ImageBuf *&sourceBuf, OpenImageIO::ImageSpec spec )
{
    OIIO_NAMESPACE_USING;
    int map[] = { 0, 1, 2, 3 };
    float fill[] = { 0, 0, 0, 255 };

    CHECK( sourceBuf ) << "NULL Pointer passed to channels()";

    // return a copy of the original if its the correct size.
    ImageSpec sourceSpec = sourceBuf->specmod();
    if( sourceBuf->spec().nchannels == spec.nchannels ) return;

    if( sourceSpec.nchannels < 4 ) map[3] = -1;

    // Otherwise re-order channels
    ImageBuf *tempBuf = new ImageBuf;
    ImageBufAlgo::channels( *tempBuf, *sourceBuf, spec.nchannels, map, fill );
    sourceBuf->clear();
    delete sourceBuf;
    sourceBuf = tempBuf;
}

void
swizzle( OpenImageIO::ImageBuf *&sourceBuf )
{
    OIIO_NAMESPACE_USING;

    int map[] = { 2, 1, 0, 3 };
    float fill[] = { 0, 0, 0, 255 };

    CHECK( sourceBuf ) << "NULL Pointer passed to swizzle()";

    if( sourceBuf->spec().nchannels < 4 ) map[3] = -1;
    ImageBuf *tempBuf = new ImageBuf;
    ImageBufAlgo::channels( *tempBuf, *sourceBuf, 4, map, fill );

    sourceBuf->clear();
    delete sourceBuf;
    sourceBuf = tempBuf;
}

void
progressBar( std::string message, float goal, float progress )
{
    static std::stringstream text;
    uint32_t columns = getenv( "COLUMNS" ) ? atoi( getenv( "COLUMNS" ) ) : 80;
    float ratio = progress / goal;
    uint32_t barSize = columns - message.length() - 6;

    //construct the bar
    text.str( std::string() ); // wipe the bar of content
    text << "\033[0G\033[2K"; // clear the current line and set the cursor position at the beginning
    for( uint32_t i = 0; i < columns - 6; ++i) text << "-"; //fill the output with a single dotten line
    text << "\033[G"; // reset the cursor position;
    text << message; // put the header on
    for( uint32_t i = 0; i < ratio * barSize; ++i) text << "#"; // fill in current progress
    text << ">\033[" << columns - 5 << "G[" << (int)(ratio*100) << "%]"; // put the end cap on

    if( ratio < 1.0f ) text << "\033[F"; // if were not done, move the cursor back up.

    LOG( INFO ) << text.str();

    std::cout.flush();
}