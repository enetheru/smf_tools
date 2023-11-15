#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <filesystem>
#include <nlohmann/json.hpp>

class TileMap {
    std::vector< uint32_t > _data;
    uint32_t _width{};
    uint32_t _height{};
public:

    // constructors
    TileMap( ) = default;
    TileMap( uint32_t width, uint32_t height );
    TileMap( uint32_t width, uint32_t height, const std::function <uint32_t(uint32_t, uint32_t)>& generator );

    // Assignment
    TileMap &operator=( const TileMap &rhs );

    //size
    void setSize( uint32_t width, uint32_t height );
    [[nodiscard]] auto size() const -> std::pair<uint32_t, uint32_t> { return {_width, _height }; }
    [[nodiscard]] auto width() const -> uint32_t { return _width; }
    [[nodiscard]] auto height() const -> uint32_t { return _height; }
    [[nodiscard]] auto length() const -> uint32_t { return _data.size();}

    //access
    [[nodiscard]] uint32_t getXY( uint32_t x, uint32_t y ) const ;
    [[nodiscard]] uint32_t getI( uint32_t index ) const ;
    [[nodiscard]] uint32_t *data() { return _data.data(); }
    void setXY( uint32_t x, uint32_t y, uint32_t value );
    void setI( uint32_t index, uint32_t value );

    [[nodiscard]] nlohmann::ordered_json json() const ;
    [[nodiscard]] std::string csv() const ;


    static TileMap fromCSV( const std::filesystem::path &filePath );
    static TileMap fromJSON( const std::filesystem::path &filePath );
    static TileMap fromSMF( const std::filesystem::path &filePath );
};