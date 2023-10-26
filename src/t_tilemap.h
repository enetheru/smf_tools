#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <filesystem>

class TileMap {
    std::vector< uint32_t > _map;
    uint32_t _width{};
    uint32_t _height{};
public:

    // constructors
    TileMap( ) = default;
    TileMap( uint32_t width, uint32_t height );

    // Assignment
    TileMap &operator=( const TileMap &rhs);

    //csv
    static TileMap *createCSV( const std::filesystem::path& filePath );
    void fromCSV( const std::filesystem::path& filePath );
    std::string toCSV();

    //size
    void setSize( uint32_t width, uint32_t height );
    [[nodiscard]] auto size() const -> std::pair<uint32_t, uint32_t> { return {_width, _height }; }
    [[nodiscard]] auto width() const -> uint32_t { return _width; }
    [[nodiscard]] auto height() const -> uint32_t { return _height; }
    [[nodiscard]] auto length() const -> uint32_t { return _map.size();}

    //generation
    void consecutive( );

    //access
    [[nodiscard]] uint32_t getXY( uint32_t x, uint32_t y ) const ;
    [[nodiscard]] uint32_t getI( uint32_t index ) const ;
    uint32_t *data() { return _map.data(); }
    void setXY( uint32_t x, uint32_t y, uint32_t value );
    void setI( uint32_t index, uint32_t value );
};

