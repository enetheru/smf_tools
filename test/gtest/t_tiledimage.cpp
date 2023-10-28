#include <gtest/gtest.h>

#include "tiledimage.h"

TEST( tiledimage, default_constructor ) {
    TiledImage tiledImage;

    const auto &imageSpec = tiledImage.getImageSpec();
    EXPECT_EQ( imageSpec.width, 0 );
    EXPECT_EQ( imageSpec.height, 0 );
}

TEST( tiledimage, constructor_cache_map ){
    TileCache tileCache;
    TileMap tileMap;
    TiledImage tilesImage( tileCache, tileMap );
}

TEST( tiledimage, constructor_size ) {
    TiledImage tiledImage( {1024, 1024, 4, OIIO::TypeDesc::UINT8},
                           32, 32, 0 );


}

TEST( tiledimage, setSize) {}
TEST( tiledimage, setTileSize) {}

TEST( tiledimage, setOverlap) {}

TEST( tiledimage, setTileMap) {}
TEST( tiledimage, squareFromCache ) {}

TEST( tiledimage, getRegion ) {}
TEST( tiledimage, getUVRegion ) {}
TEST( tiledimage, getTile ) {}

TEST( tiledimage, setTSpec) {}
TEST( tiledimage, getTileSpec ) {}