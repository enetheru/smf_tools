#ifndef TILEMAP_H
#define TILEMAP_H

#include <cstdint>
#include <vector>
#include <string>

class TileMap
{
public:
    uint32_t width = 1;
    uint32_t height = 1;
    std::vector< uint32_t > map;

    TileMap( );
    TileMap( uint32_t w, uint32_t h );
    TileMap( std::string fileName );

    std::string toCSV();
    uint32_t *data();
    void setSize( uint32_t w, uint32_t h );

    uint32_t &operator() ( uint32_t y, uint32_t x );
    uint32_t &operator() ( uint32_t idx );
};

#endif //TILEMAP_H
