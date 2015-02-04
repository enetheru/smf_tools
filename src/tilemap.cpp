#include <cstdint>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <stdexcept>

#include "elog/elog.h"

#include "tilemap.h"


// CONSTRUCTORS
// ============

TileMap::TileMap( )
{ }

TileMap::TileMap( uint32_t width,  uint32_t height )
    : _width( width ), _height( height )
{
    map.resize( width * height , 0 );
}

TileMap *
TileMap::createCSV( std::string fileName )
{
    std::fstream file( fileName, std::ios::in );
    if(! file.good() ) return NULL;
    file.close();

    TileMap *tileMap = new TileMap;
    tileMap->fromCSV( fileName );
    return tileMap;
}

// Copy Constructor
TileMap::TileMap( const TileMap& rhs)
:   _width( rhs.width ), _height( rhs.height )
{
    map = rhs.map;
}

// Assignment operator
TileMap &
TileMap::operator=( const TileMap &rhs)
{
    _width = rhs.width;
    _height = rhs.height;
    map = rhs.map;
    return *this;
}

// IMPORT
// ======
void
TileMap::fromCSV( std::string fileName )
{
    // reset
    _width = _height = 0;
    // setup
    std::string cell;
    std::stringstream line;
    std::vector< std::string > tokens;
    std::fstream file( fileName, std::ios::in );
    CHECK( file.good() ) << "cannot open " << fileName;

    // get dimensions
    if( std::getline( file, cell ) ) ++_height;
    else return;

    line.str( cell );
    while( std::getline( line, cell, ',' ) ) ++_width;
    while( std::getline( file, cell ) ) ++_height;

    //reserve space to avoid many memory allocations
    map.resize( width * height );

    // convert csv
    file.clear();
    file.seekg(0);
    uint32_t y = 0;
    while( std::getline( file, cell ) ){
        line.clear();
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
TileMap::setSize( uint32_t width, uint32_t height )
{
    CHECK( width > 0 ) << "Width must be > 0";
    CHECK( height > 0 ) << "Height must be ";

    _width = width; _height = height;
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


