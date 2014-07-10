#include "util.h"

// Evaluates a string into two integers
/* The function takes a string in the form of IxK, and assignes
 * the values of I,K to x,y.
 */
void valxval( string s, unsigned int &x, unsigned int &y ){
    unsigned int d;
    d = s.find_first_of( 'x', 0 );

    if(d) x = stoi( s.substr( 0, d ) );
    else x = 0;

    if(d == s.size()-1 ) y = 0;
    else y = stoi( s.substr( d + 1, string::npos) );
}

/// Expand a string sequence of integers to a vector
/*  The function takes a string in the form of comma ',' separated values.
 *  When two values are separated with a dash '-' then all the numbers
 *  inbetween are expanded.\n
 *  ie '1,2,3-7,2-5' = {1,2,3,4,5,6,7,2,3,4,5}
 */
vector< unsigned int > expandString( const char *s ){
    vector< unsigned int > result;

    int start;
    bool sequence = false;
    const char *begin;
    do {
        begin = s;

        while( *s != ',' && *s != '-' && *s ) s++;
        if( begin == s) continue;

        if( sequence ){
            for( int i = start; i < stoi( string( begin, s ) ); ++i )
                result.push_back( i );
        }

        if( *(s) == '-' ){
            sequence = true;
            start = stoi( string( begin, s ) );
        } else {
            sequence = false;
            result.push_back( stoi( string( begin, s ) ) );
        }
    }
    while( *s++ != '\0' );

    return result;
}

/// Scales an ImageBuf according to a given ImageSpec
/*  If sourceBuf is NULL then return a blank image.
 */
ImageBuf *scale( ImageBuf *sourceBuf, ImageSpec spec ){
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

/// re-orders the channels an imageBuf according to the imagespec
/*  Returns a copy of the souceBuf with the number of channels in
 *  the ImageSpec spec. if there is no Alpha than a opaque one is created.
 *  If sourceBuf is NULL then a blank image is returned.
 */
ImageBuf *channels( ImageBuf *sourceBuf, ImageSpec spec ){
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


