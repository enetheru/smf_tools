#ifndef TILEMAP_H
#define TILEMAP_H

#include <cstdint>
#include <vector>
#include <string>

class TileMap
{
public:
    // data members
    uint32_t width;
    uint32_t height;
    std::vector< uint32_t > map;

    // constructors
    TileMap( );
    TileMap( uint32_t w, uint32_t h );
    TileMap( std::string fileName );

    //import
    void fromCSV( std::string fileName );
    
    //export
    std::string toCSV();

    //modification
    void setSize( uint32_t w, uint32_t h );

    //generation
    void consecutive( );

    //access
    uint32_t &operator() ( uint32_t y, uint32_t x );
    uint32_t &operator() ( uint32_t idx );
    uint32_t *data();
};

#endif //TILEMAP_H
