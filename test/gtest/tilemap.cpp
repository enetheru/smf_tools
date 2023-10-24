//
// Created by nicho on 24/10/2023.
//
#include <gtest/gtest.h>
#include "tilemap.h"

/* TODO
    // constructors
    static TileMap *createCSV( std::filesystem::path filePath );

    //import
    void fromCSV( std::filesystem::path filePath );

    //export
    std::string toCSV();

    //access
    uint32_t &operator() ( uint32_t idx );
    uint32_t *data();
};
 */

TEST( tilemap, default_constructor ){
    TileMap tileMap;
    auto [width, height] = tileMap.size();
    auto length = tileMap.length();
    ASSERT_EQ( width, 0 );
    ASSERT_EQ( height, 0 );
    ASSERT_EQ( length, 0 );
}

TEST( tilemap, constructor ){
    TileMap tileMap(3,3);
    auto [width, height] = tileMap.size();
    auto length = tileMap.length();
    ASSERT_EQ( width, 3 );
    ASSERT_EQ( height, 3 );
    ASSERT_EQ( length, 9 );
}

TEST( tilemap, assignment ){
    TileMap tileMap2(3,3);
    TileMap tileMap = tileMap2;
    auto [width, height] = tileMap.size();
    auto length = tileMap.length();
    ASSERT_EQ( width, 3 );
    ASSERT_EQ( height, 3 );
    ASSERT_EQ( length, 9 );
}

TEST( tilemap, setSize ){
    TileMap tileMap;
    tileMap.setSize(3,3);
    auto [width, height] = tileMap.size();
    auto length = tileMap.length();
    ASSERT_EQ( width, 3 );
    ASSERT_EQ( height, 3 );
    ASSERT_EQ( length, 9 );
}

TEST( tilemap, generation ){
    TileMap tileMap;
    tileMap.setSize(3,3);
    tileMap.consecutive();
    auto [width, height] = tileMap.size();
    auto length = tileMap.length();
    ASSERT_EQ( width, 3 );
    ASSERT_EQ( height, 3 );
    ASSERT_EQ( length, 9 );
    ASSERT_EQ( tileMap.getXY(0,0), 0 );
    ASSERT_EQ( tileMap.getXY(0,2), 6 );
    ASSERT_EQ( tileMap.getXY(2,2), 8 );
}