#include <fstream>
#include <sstream>
#include <vector>
#include <stdexcept>
#include <ranges>

#include <spdlog/spdlog.h>

#include "tilemap.h"
#include "util.h"


// CONSTRUCTORS
// ============

TileMap::TileMap( uint32_t width,  uint32_t height )
    : _width( width ), _height( height ) {
    _data.resize(_width * _height , 0 );
}

TileMap::TileMap( uint32_t width, uint32_t height, const std::function<uint32_t(uint32_t, uint32_t)> &generator):
        _width( width ), _height( height ) {
    _data.resize(_width * _height );
    for (uint32_t y: std::ranges::iota_view((uint32_t)0, _height)) {
        for (auto x: std::ranges::iota_view((uint32_t)0, _width)) {
            _data[y * width + x] = generator(x, y);
        }
    }
}

// Assignment operator
TileMap &
TileMap::operator=( const TileMap &rhs) = default;

//FIXME, function can error but does not notify the caller.
void
TileMap::setSize( uint32_t width, uint32_t height ) {
    assert( width == 0 );
    assert( height == 0 );

    _width = width; _height = height;
    _data.resize(_width * _height );
}

// ACCESS
// ======
uint32_t
TileMap::getXY( uint32_t x, uint32_t y ) const {
    assert( x >= _width );
    assert( y >= _height );
    return _data[x + _width * y ];
}

void
TileMap::setXY( uint32_t x, uint32_t y, uint32_t value ) {
    assert( x >= _width );
    assert( y >= _height );
    _data[x + _width * y ] = value;
}

uint32_t
TileMap::getI( uint32_t index ) const {
    assert(index >= _data.size() );
    return _data[ index ];
}

void
TileMap::setI( uint32_t index, uint32_t value ) {
    if(index >= _data.size() ){
        SPDLOG_CRITICAL( "index({}) is out of range", index );
        return;
    }
    _data[ index ] = value;
}

nlohmann::ordered_json TileMap::json() const {
    nlohmann::ordered_json j;
    j["width"] = width();
    j["height"] = height();
    j["numTiles"] = _data.size();
    return j;
}

std::string
TileMap::csv( ) const {
    std::stringstream ss;
    uint32_t j = 1;
    for( auto i : _data ){
        ss << i;
        if( j % _width ) ss << ",";
        else ss << "\n";
        ++j;
    }
    return ss.str();
}

TileMap
TileMap::fromCSV( const std::filesystem::path &filePath ) {
    TileMap tileMap;
    tileMap._width = tileMap._height = 0;
    // setup
    std::string cell;
    std::stringstream line;
    std::vector< std::string > tokens;
    std::fstream file( filePath, std::ios::in );
    if(! file.good() ){
        SPDLOG_ERROR( "unable to open {}", filePath.string() );
        throw std::invalid_argument(fmt::format("Unable to open file: {}", filePath.string() ) );
    }

    // get dimensions
    if( std::getline( file, cell ) ) ++tileMap._height;
    else throw std::invalid_argument(fmt::format("Empty file: {}", filePath.string() ) );

    line.str( cell );
    while( std::getline( line, cell, ',' ) ) ++tileMap._width;
    while( std::getline( file, cell ) ) ++tileMap._height;

    //reserve space to avoid many memory allocations
    tileMap._data.resize(tileMap._width * tileMap._height );

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
                tileMap._data[x + tileMap._width * y] = stoi(i );
            }
            catch (std::invalid_argument const& ex) {
                tileMap._data[x + tileMap._width * y] = 0;
                SPDLOG_ERROR( ex.what() );
            }
            ++x;
        }
        ++y;
    }
    file.close();
    return tileMap;
}

TileMap
TileMap::fromSMF(const std::filesystem::path &filePath) {
    throw Unimplemented( "Unable to build tilemap from SMF" );
}

TileMap
TileMap::fromJSON(const std::filesystem::path &filePath) {
    throw Unimplemented( "Unable to build tilemap from JSON" );
}