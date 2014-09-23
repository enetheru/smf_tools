#include "tilemap.h"

#include "elog/elog.h"
#include <cstdint>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <stdexcept>

// CONSTRUCTORS
// ============

TileMap::TileMap( )
{ }

TileMap::TileMap( uint32_t w,  uint32_t h )
    : width( w ), height( h )
{
    map.resize( width * height , 0 );
}

TileMap::TileMap( std::string fileName )
{
    fromCSV( fileName );
}

// IMPORT
// ======
void
TileMap::fromCSV( std::string fileName )
{
    // reset
    width = height = 0;
    // setup
    std::string cell;
    std::stringstream line;
    std::vector< std::string > tokens;
    std::fstream file( fileName, std::ios::in );
    CHECK(! file.good() ) << "cannot open " << fileName;

    // get dimensions
    if( std::getline( file, cell ) ) ++height;
    else return;

    line.str( cell );
    while( std::getline( line, cell, ',' ) ) ++width;
    while( std::getline( file, cell ) ) ++height;

    //reserve space to avoid many memory allocations
    map.resize( width * height );

    // convert csv
    file.seekg(0);
    uint32_t y = 0;
    while( std::getline( file, cell ) ){
        line.str( cell );

        tokens.clear();
        while( std::getline( line, cell, ',' ) ) tokens.push_back( cell );

        uint32_t x = 0;
        for( auto i = tokens.begin(); i != tokens.end(); ++i ){
            try {
                map[x + width * y] = stoi( *i );
            }
            catch( std::invalid_argument ){
                map[x + width * y] = 0;
            }
            ++x;
        }
        ++y;
    }
    file.close();
}

std::string
TileMap::toCSV( )
{
    std::stringstream ss;
    uint32_t j = 1;
    for( auto i = map.begin(); i != map.end(); ++i ){
        ss << *i;
        if( j % width ) ss << ",";
        else ss << "\n";
        ++j;
    }
    return ss.str();
}

void
TileMap::setSize( uint32_t w, uint32_t h )
{
    CHECK(! w ) << "Width must be >= 1";
    CHECK(! h ) << "Height must be >= 1";

    width = w; height = h;
    map.resize( width * height );
}

// GENERATION
// ==========

void
TileMap::consecutive( )
{
    for( uint32_t i = 0; i < map.size(); ++i ){
        map[ i ] = i;
    }
}

// ACCESS
// ======
uint32_t &
TileMap::operator() ( uint32_t x, uint32_t y )
{
    CHECK( x < width ) << x << " > " << width;
    CHECK( y < height ) << y << " > " << height;
    return map[ x + width * y ];
}

uint32_t &
TileMap::operator() ( uint32_t idx )
{
    CHECK( idx >= map.size() ) << idx << " out of range";
    return map[idx];
}

uint32_t *
TileMap::data( )
{
    return map.data();
}


