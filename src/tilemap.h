#ifndef TILEMAP_H
#define TILEMAP_H

#include <vector>
#include <string>

class TileMap{
public:
    unsigned int width = 0;
    unsigned int height = 0;
    std::vector< unsigned int > map;

    TileMap( ){ };
    TileMap( unsigned int width, unsigned int height );
    TileMap( std::string fileName );

    std::string toCSV();
    unsigned int *data();

    unsigned int &operator() ( unsigned int row, unsigned int col );
    unsigned int &operator() ( unsigned int idx );
};

#endif //TILEMAP_H
