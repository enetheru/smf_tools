#pragma once

#include <cstdint>
#include <vector>
#include <string>

class TileMap {
    std::vector< uint32_t > _map;
public:
    // data members
    uint32_t width;
    uint32_t height;

    // constructors
    TileMap( ) = default;
    TileMap( uint32_t width, uint32_t height );

    static TileMap *createCSV( const std::string& fileName );

    TileMap( const TileMap &rhs);
    TileMap &operator=( const TileMap &rhs);

    //import
    void fromCSV( const std::string& fileName );

    //export
    std::string toCSV();

    //modification
    void setSize(uint32_t _width, uint32_t _height );

    //generation
    void consecutive( );

    //access
    uint32_t &operator() ( uint32_t y, uint32_t x );
    uint32_t &operator() ( uint32_t idx );
    uint32_t *data();

	//information
	int size() { return _map.size(); }
};

