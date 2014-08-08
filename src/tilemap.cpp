#include "tilemap.h"


#include <cstdint>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <stdexcept>

TileMap::TileMap( )
{
    map.resize( 1, 0 );
}

TileMap::TileMap( uint32_t w,  uint32_t h )
    : width( w ), height( h )
{
    map.resize( width * height , 0 );
}

TileMap::TileMap( std::string fileName )
{
    // setup
    std::string cell;
    std::stringstream line;
    std::vector< std::string > tokens;
    std::fstream file( fileName, std::ios::in );

    // get dimensions
    if( std::getline( file, cell ) ) ++height;
    else return;

    line.str( cell );
    while( std::getline( line, cell, ',' ) ) ++width;
    while( std::getline( file, cell ) ) ++height;

    //reserve space to avoid many memory allocations
    //FIXME this needs to be resize
    map.reserve( width * height );

    // convert csv to uint32_t
    file.seekg(0);
    while( std::getline( file, cell ) ){
        line.str( cell );

        tokens.clear();
        while( std::getline( line, cell, ',' ) ) tokens.push_back( cell );

        for( auto i = tokens.begin(); i != tokens.end(); ++i ){
            try {
                map.push_back( stoi( *i ) );
            }
            catch( std::invalid_argument ){
                //FIXME this needs to use array subscript
                map.push_back( 0 );
            }
        }
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

uint32_t *
TileMap::data( )
{
    return map.data();
}

void
TileMap::setSize( uint32_t w, uint32_t h )
{
    width = w; height = h;
    map.resize( width * height );
}

uint32_t &
TileMap::operator() ( uint32_t x, uint32_t y )
{
    return map[ x + width * y ];
}

uint32_t &
TileMap::operator() ( uint32_t idx )
{
    return map[idx];
}
