#include "tilemap.h"

#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <stdexcept>

TileMap::TileMap( unsigned int width,  unsigned int height )
{
    this->width = width;
    this->height = height;
    map.resize( width * height );
    return;
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
    map.reserve( width * height );

    // convert csv to unsigned int
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
    int j = 1;
    for( auto i = map.begin(); i != map.end(); ++i ){
        ss << *i;
        if( j % width ) ss << ",";
        else ss << "\n";
        ++j;
    }
    return ss.str();
}

unsigned int *
TileMap::data( )
{
    return map.data();
}

unsigned int &
TileMap::operator() ( unsigned int x, unsigned int y )
{
    return map[ x + width * y ];
}

unsigned int &
TileMap::operator() ( unsigned int idx )
{
    return map[idx];
}
