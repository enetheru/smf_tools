#pragma once

#include <cstdint>
#include <vector>
#include <string>

class TileMap
{
    uint32_t _width, _height;
    std::vector< uint32_t > map;
public:
    // data members
    const uint32_t &width = _width;
    const uint32_t &height = _height;

    // constructors
    TileMap( );
    TileMap( uint32_t width, uint32_t height );

    static TileMap *createCSV( std::string fileName );
    
    TileMap( const TileMap &rhs);
    TileMap &operator=( const TileMap &rhs);

    //import
    void fromCSV( std::string fileName );

    //export
    std::string toCSV();

    //modification
    void setSize( uint32_t width, uint32_t height );

    //generation
    void consecutive( );

    //access
    uint32_t &operator() ( uint32_t y, uint32_t x );
    uint32_t &operator() ( uint32_t idx );
    uint32_t *data();
};

