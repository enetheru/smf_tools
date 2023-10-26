#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include "t_tilecache.h"

#ifndef SOURCE_ROOT
#define SOURCE_ROOT "./"
#endif

/*
    struct TileSource{
        uint32_t iStart,iEnd;
        TileSourceType type;
        std::filesystem::path filePath;
    };

    uint32_t getNumTiles() const { return tileCount; }
    void addSource( const std::filesystem::path& filePath );
    std::optional<OIIO::ImageBuf> getTile( uint32_t index );
*/

[[maybe_unused]] static const std::filesystem::path currentPath = std::filesystem::current_path();
[[maybe_unused]] static const std::filesystem::path sourcePath( SOURCE_ROOT );
[[maybe_unused]] static const std::filesystem::path testPath( sourcePath / "test/data" );

TEST( tilecache, default_constructor ) {
    TileCache tilecache;
    ASSERT_EQ(tilecache.getNumTiles(), 0);
    ASSERT_FALSE( tilecache.getTile(0).has_value() );
}

TEST( tilecache, add_image ) {
    TileCache tileCache;
    tileCache.addSource( (testPath / "image_1.png") );
    ASSERT_EQ( tileCache.getNumTiles(), 1 );
    auto maybeTile = tileCache.getTile(0);
    ASSERT_TRUE( maybeTile.has_value() );
    auto imageBuf = maybeTile.value();
    SPDLOG_INFO("imageBuf.name(): {}", std::string(imageBuf.name()) );
    imageBuf.read();

    if(! imageBuf.write( (currentPath / "tilecache.add_image.jpg").string() ) ){
        SPDLOG_CRITICAL( imageBuf.geterror() );
        FAIL();
    }
}

TEST( tilecache, add_smt ) {
    TileCache tilecache;
}

TEST( tilecache, add_smf ) {
    TileCache tilecache;
}