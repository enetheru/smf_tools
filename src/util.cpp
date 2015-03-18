#include <chrono>
#include <cstdint>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <list>

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

    const OpenImageIO::ImageSpec srcSpec = sourceBuf->spec();
    CHECK( sourceBuf ) << " NULL pointer passed to scale()";

    // do nothing if the original if its the correct size.
    if( srcSpec.width == spec.width && srcSpec.height == spec.height ){
        return;
    }
    
    // Otherwise scale
    ROI roi(0, spec.width, 0, spec.height, 0, 1, 0, srcSpec.nchannels );
    ImageBuf *tempBuf = new ImageBuf;

    // resmple is faster but creates black outlines when scaling up, so only
    // use it for scaling down.
    if( spec.width < srcSpec.width && spec.height < srcSpec.height ){
        ImageBufAlgo::resample( *tempBuf, *sourceBuf, false, roi );
    }
    else {
        ImageBufAlgo::resize( *tempBuf, *sourceBuf, "", false, roi );
    }

    sourceBuf->clear();
    delete sourceBuf;
    sourceBuf = tempBuf;
}

void
channels( OpenImageIO::ImageBuf *&sourceBuf, OpenImageIO::ImageSpec spec )
{
    OIIO_NAMESPACE_USING;
    int map[] = { 0, 1, 2, 3 };
    float fill[] = { 0, 0, 0, 1.0 };

    CHECK( sourceBuf ) << "NULL Pointer passed to channels()";

    // return a copy of the original if its the correct size.
    if( sourceBuf->spec().nchannels == spec.nchannels ) return;
    if( sourceBuf->spec().nchannels < 4 ) map[3] = -1;
    if( sourceBuf->spec().nchannels < 3 ) map[2] = -1;
    if( sourceBuf->spec().nchannels < 2 ) map[1] = -1;

    // Otherwise update channels to spec channels
    ImageBuf *tempBuf = new ImageBuf;
    ImageBufAlgo::channels( *tempBuf, *sourceBuf, spec.nchannels, map, fill );
    sourceBuf->clear();
    delete sourceBuf;
    sourceBuf = tempBuf;
}

void convert( OpenImageIO::ImageBuf *&sourceBuf, OpenImageIO::ImageSpec spec )
{
    OIIO_NAMESPACE_USING;

    ImageBuf *tempBuf = new ImageBuf( spec );
    tempBuf->copy_pixels( *sourceBuf );
    tempBuf->read(0,0,true, spec.format);
    delete sourceBuf;
    sourceBuf = tempBuf;
}

void
swizzle( OpenImageIO::ImageBuf *&sourceBuf )
{
    OIIO_NAMESPACE_USING;

    int map[] = { 2, 1, 0, 3 };
    float fill[] = { 0, 0, 0, 1.0 };

    CHECK( sourceBuf ) << "NULL Pointer passed to swizzle()";

    if( sourceBuf->spec().nchannels < 4 ) map[3] = -1;
    ImageBuf *tempBuf = new ImageBuf;
    ImageBufAlgo::channels( *tempBuf, *sourceBuf, 4, map, fill );

    sourceBuf->clear();
    delete sourceBuf;
    sourceBuf = tempBuf;
}

void
progressBar( std::string header, float goal, float current )
{
    static uint32_t seconds;
    static uint32_t minutes;
    static uint32_t hours;
    static uint32_t average;
    static float ratio;

    static int progress;
    static int remainder;
    static int last_current = 0;

    ratio = current / goal;

    static std::list< uint32_t > history;
    if( history.size() >= 30) history.pop_front();

    using namespace std::chrono;
    static duration< double > time_span;
    static steady_clock::time_point last;
    static steady_clock::time_point now;

    now = steady_clock::now();

    time_span = duration_cast< duration< double > >(now - last);
    if( time_span.count() < 0.5 && ratio < 1 )return;
    last = now;

    progress = current - last_current;
    last_current = current;
    remainder = goal - current;

    history.push_back( progress );
    average = 0;
    for( auto i = history.begin(); i != history.end(); ++i)average += *i;
    average = average / history.size();

    seconds = remainder / average;
    hours = seconds / 3600;
    minutes = (seconds % 3600 ) / 60;
    seconds = seconds % 60;

    //construct the footer
    std::stringstream footer;
    footer << "[";
    if( hours ) footer << hours << ":"; // hours
    if( minutes ) footer << std::setfill('0') << std::setw(2) << minutes << ":"; //minutes
    footer << std::setfill('0') << std::setw(2) << seconds << "]"; // seconds
    footer << "[" << (int)(ratio * 100) << "%]"; // percentage complete

    // calculate formatting factors
    std::stringstream text;
    uint32_t columns = getenv( "COLUMNS" ) ? atoi( getenv( "COLUMNS" ) ) : 80;
    uint32_t barSize = columns - header.size() - footer.str().size();

    //construct the bar
    text << "\033[0G\033[2K"; // clear the current line and set the cursor position at the beginning
    for( uint32_t i = 0; i < columns; ++i) text << "-"; //fill the output with a single dotten line
    text << "\033[G"; // reset the cursor position;
    text << header; // put the header on
    for( uint32_t i = 0; i < ratio * barSize; ++i) text << "#"; // fill in current progress
    text << ">\033[" << columns - footer.str().size() + 1 << "G" << footer.str(); // put the end cap on
    if( ratio < 1.0f ) text << "\033[F"; // if were not done, move the cursor back up.

    std::cout << text.str() << std::endl;
}
