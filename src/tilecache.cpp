#include "tilecache.h"
#include "util.h"
#include "smt.h"


ImageBuf *
TileCache::getTile( int n )
{
    if( n > nTiles ) return NULL;

    unsigned int i = 0;
    while( map[ i ] < n ) ++i;

    if( isSMT( filenames[ i ] ) ){
        SMT smt( filenames[ i ] );
        return smt.getTile( map[ i ] - n );
    }
    else if( isImage( filenames[ i ] ) ){
        return new ImageBuf( filenames[ i ] );
    }

    return NULL;
}

void
TileCache::push_back( string s )
{
    if( isImage( s ) ){
        nTiles++;
        map.push_back( nTiles );
        filenames.push_back( s );
    }

    if( isSMT( s ) ){
        SMT smt( s );
        nTiles += smt.getNTiles();
        map.push_back( nTiles );
        filenames.push_back( s );
    }
    return;
}
