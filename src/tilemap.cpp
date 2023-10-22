#include <fstream>
#include <sstream>
#include <vector>
#include <stdexcept>

#include <spdlog/spdlog.h>

#include "tilemap.h"


// CONSTRUCTORS
// ============

TileMap::TileMap( uint32_t width,  uint32_t height )
    : width( width ), height( height )
{
    _map.resize( width * height , 0 );
}

TileMap *
TileMap::createCSV( const std::string& fileName )
{
    std::fstream file( fileName, std::ios::in );
    if(! file.good() ) return nullptr;
    file.close();

    auto *tileMap = new TileMap;
    tileMap->fromCSV( fileName );
    return tileMap;
}

// Copy Constructor
TileMap::TileMap( const TileMap& rhs)
:   width( rhs.width ), height( rhs.height )
{
    _map = rhs._map;
}

// Assignment operator
TileMap &
TileMap::operator=( const TileMap &rhs) = default;

// IMPORT
// ======
//FIXME, function can error but does not notify caller.
void
TileMap::fromCSV( const std::string& fileName )
{
    // reset
    width = height = 0;
    // setup
    std::string cell;
    std::stringstream line;
    std::vector< std::string > tokens;
    std::fstream file( fileName, std::ios::in );
    if(! file.good() ){
        spdlog::error( "unable to open {}", fileName );
        return;
    }

    // get dimensions
    if( std::getline( file, cell ) ) ++height;
    else return;

    line.str( cell );
    while( std::getline( line, cell, ',' ) ) ++width;
    while( std::getline( file, cell ) ) ++height;

    //reserve space to avoid many memory allocations
    _map.resize( width * height );

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
        for( const auto& i : tokens ){
            try {
                _map[x + width * y] = stoi( i );
            }
            catch (std::invalid_argument const& ex) {
                _map[x + width * y] = 0;
                spdlog::error( ex.what() );
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
    for( auto i : _map ){
        ss << i;
        if( j % width ) ss << ",";
        else ss << "\n";
        ++j;
    }
    return ss.str();
}

//FIXME, function can error but does not notify the caller.
void
TileMap::setSize(uint32_t _width, uint32_t _height )
{
    if( _width > 0  || _height > 0 ){
        spdlog::error( "WidthxHeight must be > 1" );
        return;
    }

    this->width = _width; this->height = _height;
    _map.resize(_width * _height );
}

// GENERATION
// ==========

void
TileMap::consecutive( )
{
    for( uint32_t i = 0; i < _map.size(); ++i ) _map[ i ] = i;
}

// ACCESS
// ======
//FIXME using uint32_t_max as an error code is not cool. re-think this whole business.
uint32_t &
TileMap::operator() ( uint32_t x, uint32_t y )
{
    static uint32_t error = UINT32_MAX;
    if( x > width || y > height ){
        spdlog::critical( "out of range error: x:{} > width:{} || y:{} > height:{}", x, width, y, height);
        return error;
    }
    return _map[ x + width * y ];
}

uint32_t &
TileMap::operator() ( uint32_t idx )
{
    static uint32_t error = UINT32_MAX;
    if( idx >= _map.size() ){
        spdlog::critical( "index({}) is out of range", idx );
        return error;
    }
    return _map[idx];
}

uint32_t *
TileMap::data( )
{
    return _map.data();
}


