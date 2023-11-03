#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include "tilemap.h"

/* TODO
    std::string toCSV();
    uint32_t &operator() ( uint32_t idx );
    uint32_t *data();
 */

[[maybe_unused]] static const std::filesystem::path currentPath = std::filesystem::current_path();
[[maybe_unused]] static const std::filesystem::path sourcePath( SOURCE_ROOT );
[[maybe_unused]] static const std::filesystem::path testPath( sourcePath / "test/data" );

TEST( tilemap, default_constructor ){
    TileMap tileMap;
    auto [width, height] = tileMap.size();
    auto length = tileMap.length();
    EXPECT_EQ( width, 0 );
    EXPECT_EQ( height, 0 );
    EXPECT_EQ( length, 0 );
}

TEST( tilemap, constructor ){
    TileMap tileMap(3,3);
    auto [width, height] = tileMap.size();
    auto length = tileMap.length();
    EXPECT_EQ( width, 3 );
    EXPECT_EQ( height, 3 );
    EXPECT_EQ( length, 9 );
}

TEST( tilemap, assignment ){
    TileMap tileMap2(3,3);
    TileMap tileMap = tileMap2;
    auto [width, height] = tileMap.size();
    auto length = tileMap.length();
    EXPECT_EQ( width, 3 );
    EXPECT_EQ( height, 3 );
    EXPECT_EQ( length, 9 );
}

TEST( tilemap, setSize ){
    TileMap tileMap;
    tileMap.setSize(3,3);
    auto [width, height] = tileMap.size();
    auto length = tileMap.length();
    EXPECT_EQ( width, 3 );
    EXPECT_EQ( height, 3 );
    EXPECT_EQ( length, 9 );
}

TEST( tilemap, generation ){
    TileMap tileMap;
    tileMap.setSize(3,3);
    tileMap.consecutive();
    auto [width, height] = tileMap.size();
    auto length = tileMap.length();
    EXPECT_EQ( width, 3 );
    EXPECT_EQ( height, 3 );
    EXPECT_EQ( length, 9 );
    EXPECT_EQ( tileMap.getXY(0,0), 0 );
    EXPECT_EQ( tileMap.getXY(0,2), 6 );
    EXPECT_EQ( tileMap.getXY(2,2), 8 );
}

TEST( tilemap, from_csv ){
    TileMap tileMap;
    tileMap.fromCSV( testPath / "tilemap.csv" );
    EXPECT_EQ( tileMap.length(), 4096 );
    EXPECT_EQ( tileMap.width(), 64 );
    EXPECT_EQ( tileMap.height(), 64 );
    spdlog::info( tileMap.toCSV() );
}