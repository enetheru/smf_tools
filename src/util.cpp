#include "util.h"
#include "elog/elog.h"

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <cstdint>
#include <string>
#include <vector>

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

OpenImageIO::ImageBuf *
scale( OpenImageIO::ImageBuf *sourceBuf, OpenImageIO::ImageSpec spec )
{
    OIIO_NAMESPACE_USING;
    ImageBuf *resultBuf = NULL;

    // Guarantee that a correct sized image is returned regardless of the input.
    if(! sourceBuf ) return (resultBuf = new ImageBuf( spec ));

    // return a copy of the original if its the correct size.
    ImageSpec sourceSpec = sourceBuf->specmod();
    if( sourceSpec.width == spec.width && sourceSpec.height == spec.height ){
        resultBuf = new ImageBuf;
        resultBuf->copy( *sourceBuf );
        return resultBuf;
    }

    // Otherwise scale
    ROI roi(0, spec.width, 0, spec.height, 0, 1, 0, sourceSpec.nchannels);
    resultBuf = new ImageBuf;
    ImageBufAlgo::resample( *resultBuf, *sourceBuf, false, roi );
#ifdef DEBUG_IMG
    static int i = 0;
    resultBuf->write("SMF::scale_" + to_string(i) + ".tif", "tif");
    i++;
#endif //DEBUG_IMG
    return resultBuf;
}

OpenImageIO::ImageBuf *
channels( OpenImageIO::ImageBuf *sourceBuf, OpenImageIO::ImageSpec spec )
{
    OIIO_NAMESPACE_USING;
    ImageBuf *resultBuf = NULL;
    int map[] = {0,1,2,3,4};
    float fill[] = {0,0,0,255};

    // Guarantee that a correct sized image is returned regardless of the input.
    if(! sourceBuf ) return (resultBuf = new ImageBuf( spec ));

    // return a copy of the original if its the correct size.
    ImageSpec sourceSpec = sourceBuf->specmod();
    if( sourceSpec.nchannels == spec.nchannels ){
        resultBuf = new ImageBuf;
        resultBuf->copy( *sourceBuf );
        return resultBuf;
    }
    if( sourceSpec.nchannels < 4 ) map[3] = -1;

    // Otherwise re-order channels
    resultBuf = new ImageBuf;
    ImageBufAlgo::channels( *resultBuf, *sourceBuf, spec.nchannels, map, fill );
#ifdef DEBUG_IMG
    static int i = 0;
    resultBuf->write("SMF::channels_" + to_string(i) + ".tif", "tif");
    i++;
#endif //DEBUG_IMG
    return resultBuf;
}


