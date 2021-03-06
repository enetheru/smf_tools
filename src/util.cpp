#include <chrono>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <list>

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>

#include <elog.h>

#include "util.h"

std::pair< uint32_t, uint32_t >
valxval( const std::string input )
{
    std::pair< uint32_t, uint32_t> result;

    auto d = input.find_first_of( 'x', 0 );

    if( d ) result.first = std::stoi( input.substr( 0, d ) );
    else result.first = 0;

    if( d == input.size() - 1 ) result.second = 0;
    else result.second = std::stoi( input.substr( d + 1, std::string::npos) );
    return result;
}

std::vector< uint32_t >
expandString( const char *s )
{
    std::vector< uint32_t > result;
    int start = 0;
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

std::unique_ptr< OpenImageIO::ImageBuf >
fix_channels(
    std::unique_ptr< OpenImageIO::ImageBuf> && inBuf,
    const OpenImageIO::ImageSpec &spec )
{
    OIIO_NAMESPACE_USING;

    int map[] = { 0, 1, 2, 3 };
    static const float fill[] = { 0, 0, 0, 1.0 };

    CHECK( inBuf ) << "nullptr passed to fix_channels()";

    // return a copy of the original if its the correct size.
    if( inBuf->spec().nchannels == spec.nchannels ) return std::move( inBuf );
    if( inBuf->spec().nchannels < 4 ) map[3] = -1;
    if( inBuf->spec().nchannels < 3 ) map[2] = -1;
    if( inBuf->spec().nchannels < 2 ) map[1] = -1;

    // Otherwise update channels to spec channels
    std::unique_ptr< OpenImageIO::ImageBuf > outBuf( new OpenImageIO::ImageBuf );
    ImageBufAlgo::channels( *outBuf, *inBuf, spec.nchannels, map, fill );
    return outBuf;
}

void
channels( OpenImageIO::ImageBuf *&sourceBuf, OpenImageIO::ImageSpec spec )
{
    OIIO_NAMESPACE_USING;
    int map[] = { 0, 1, 2, 3 };
    float fill[] = { 0, 0, 0, 1.0 };

    CHECK( sourceBuf ) << "nullptr passed to channels()";

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

std::unique_ptr< OpenImageIO::ImageBuf >
fix_scale(
    std::unique_ptr< OpenImageIO::ImageBuf> && inBuf,
    const OpenImageIO::ImageSpec &spec )
{
    OIIO_NAMESPACE_USING;
    using std::unique_ptr;

    CHECK( inBuf ) << "nullptr passed to fix_scale()";

    // return the inBuf if no change is required.
    if( (inBuf->spec().width  == spec.width )
     && (inBuf->spec().height == spec.height) ){
        DLOG( INFO ) << "no scale required";
            return std::move( inBuf );
    }

    // Otherwise scale
    ROI roi(0, spec.width, 0, spec.height, 0, 1, 0, inBuf->spec().nchannels );
    unique_ptr< ImageBuf > outBuf( new ImageBuf );

    // BUG with workaround
    // resample is faster but creates black outlines when scaling up, so only
    // use it for scaling down.
    if( (spec.width  < inBuf->spec().width )
     && (spec.height < inBuf->spec().height) ){
        ImageBufAlgo::resample( *outBuf, *inBuf, false, roi );
    }
    else {
        ImageBufAlgo::resize( *outBuf, *inBuf, "", false, roi );
    }

    return outBuf;
}

//REMOVE
void
scale( OpenImageIO::ImageBuf *&sourceBuf, OpenImageIO::ImageSpec spec )
{
    OIIO_NAMESPACE_USING;

    const OpenImageIO::ImageSpec srcSpec = sourceBuf->spec();
    CHECK( sourceBuf ) << "nullptr passed to scale()";

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
    for( auto i : history ) average += i;
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

std::string
image_to_hex( const uint8_t *data, int width, int height, int type ){
    // type 0 = UINT8
    // type 1 = DXT1
    // for now lets just do the red channel
    std::stringstream ss;
    if( type == 0 ){//RGBA8
        for( int j = 0; j < height; ++j ){
            for( int i = 0; i < width; ++i ){
                ss << std::hex << std::setfill( '0' ) << std::setw( 2 )
                        << +(uint8_t)data[ (j * width + i) * 4 ];
            }
            ss << "\n";
        }
    }
    if( type == 1 ){ //DXT1
        //in DXT1 each 4x4 block of pixels are represented by 64 bits
        //i'm really only concerned if the data exist rather than how to
        //represent it in characters.
        //i want to put the dxt1 blocks over 2 lines like:
        //ABCD ABCD ABCD ABCD
        //DEFG DEFG DEFG DEFG
        //
        //ABCD etc.
        //DEFG etc.
        //so we have to 2nd half of the bytes until a second iteration through the material

        //we treat each 4x4 block as a separate element
        width = width / 4;
        height = height / 4;

        for( int j = 0; j < height; ++j ){// rows
            for( int q = 0; q < 4; ++q ){ // sub rows
                for( int i = 0; i < width; ++i ){ // columns
                    int datapos = (j * width + i) * 8 + q*2; // every 64 bits
                    ss << std::hex << std::setfill( '0' ) << std::setw( 2 )
                       << +(uint8_t)data[ datapos ]
                       << std::hex << std::setfill( '0' ) << std::setw( 2 )
                       << +(uint8_t)data[ datapos + 1 ] << "  ";
                }
                ss << '\n';
            }
            ss << '\n';
        }

    }
    return ss.str();
}
