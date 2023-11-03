#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include "smflib/tiledimage.h"

[[maybe_unused]] static const std::filesystem::path currentPath = std::filesystem::current_path();
[[maybe_unused]] static const std::filesystem::path sourcePath( SOURCE_ROOT );
[[maybe_unused]] static const std::filesystem::path testPath( sourcePath / "test/data" );

TEST( tiledimage, default_constructor ) {
    TiledImage tiledImage;

    const auto &imageSpec = tiledImage.getImageSpec();
    EXPECT_EQ( imageSpec.width, 0 );
    EXPECT_EQ( imageSpec.height, 0 );
}

TEST( tiledimage, constructor_cache_map ){

    TileCache tileCache;
    tileCache.addSource( testPath / "data.smt" );
    SPDLOG_INFO( "TileCache: {}", tileCache.info() );

    TileMap tileMap;
    tileMap.fromCSV( testPath / "tilemap.csv" );
    SPDLOG_INFO( "TileMap: {}", tileMap.json().dump(4) );

    TiledImage tiledImage( tileCache, tileMap );
    auto imageBuf = tiledImage.getUVRegion({0, 1, 0, 1} );
    imageBuf.write("tiledimage-con_cache_map.jpg" );
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