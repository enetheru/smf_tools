#include "util.h"

void
valxval( string s, unsigned int &x, unsigned int &y)
{
    unsigned int d;
    d = s.find_first_of( 'x', 0 );

    if(d) x = stoi( s.substr( 0, d ) );
    else x = 0;

    if(d == s.size()-1 ) y = 0;
    else y = stoi( s.substr( d + 1, string::npos) );
}

vector<unsigned int>
expandString( const char *s )
{
    vector<unsigned int> result;

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
