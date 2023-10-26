#include <fstream>
#include <sstream>
#include <vector>
#include <stdexcept>

#include <spdlog/spdlog.h>

#include "t_tilemap.h"


// CONSTRUCTORS
// ============

TileMap::TileMap( uint32_t width,  uint32_t height )
    : _width( width ), _height( height )
{
    _map.resize( _width * _height , 0 );
}

TileMap *
TileMap::createCSV( const std::filesystem::path& filePath )
{
    std::fstream file( filePath, std::ios::in );
    if(! file.good() ) return nullptr;
    file.close();

    auto *tileMap = new TileMap;
    tileMap->fromCSV( filePath );
    return tileMap;
}

// Assignment operator
TileMap &
TileMap::operator=( const TileMap &rhs) = default;

// IMPORT
// ======
//FIXME, function can error but does not notify caller.
void
TileMap::fromCSV( const std::filesystem::path& filePath )
{
    // reset
    _width = _height = 0;
    // setup
    std::string cell;
    std::stringstream line;
    std::vector< std::string > tokens;
    std::fstream file( filePath, std::ios::in );
    if(! file.good() ){
        SPDLOG_ERROR( "unable to open {}", filePath.string() );
        return;
    }

    // get dimensions
    if( std::getline( file, cell ) ) ++_height;
    else return;

    line.str( cell );
    while( std::getline( line, cell, ',' ) ) ++_width;
    while( std::getline( file, cell ) ) ++_height;

    //reserve space to avoid many memory allocations
    _map.resize( _width * _height );

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
                _map[x + _width * y] = stoi( i );
            }
            catch (std::invalid_argument const& ex) {
                _map[x + _width * y] = 0;
                SPDLOG_ERROR( ex.what() );
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
        if( j % _width ) ss << ",";
        else ss << "\n";
        ++j;
    }
    return ss.str();
}

//FIXME, function can error but does not notify the caller.
void
TileMap::setSize( uint32_t width, uint32_t height ) {
    if( width == 0 || height == 0 ){
        SPDLOG_ERROR( "WidthxHeight must be > 1" );
        return;
    }

    _width = width; _height = height;
    _map.resize(_width * _height );
}

// GENERATION
// ==========

void
TileMap::consecutive( ) {
    for( uint32_t i = 0; i < _map.size(); ++i ) _map[ i ] = i;
}

// ACCESS
// ======
//FIXME using uint32_t_max as an error code is not cool. re-think this whole business.
uint32_t
TileMap::getXY( uint32_t x, uint32_t y ) const {
    if( x >= _width || y >= _height ){
        SPDLOG_CRITICAL( "out of range error: x:{} > _width:{} || y:{} > height:{}", x, _width, y, _height);
        return UINT32_MAX;
    }
    return _map[ x + _width * y ];
}

void
TileMap::setXY( uint32_t x, uint32_t y, uint32_t value ) {
    if( x > _width || y > _height ){
        SPDLOG_CRITICAL( "out of range error: x:{} > _width:{} || y:{} > height:{}", x, _width, y, _height);
        return;
    }
    _map[ x + _width * y ] = value;
}

uint32_t
TileMap::getI( uint32_t index ) const {
    static uint32_t error = UINT32_MAX;
    if( index >= _map.size() ){
        SPDLOG_CRITICAL( "index({}) is out of range", index );
        return error;
    }
    return _map[ index ];
}

void
TileMap::setI( uint32_t index, uint32_t value ) {
    if( index >= _map.size() ){
        SPDLOG_CRITICAL( "index({}) is out of range", index );
        return;
    }
    _map[ index ] = value;
}


