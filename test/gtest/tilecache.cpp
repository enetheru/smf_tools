#include <gtest/gtest.h>
#include "tilecache.h"

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

TEST( tilecache, default_constructor ) {
    TileCache tilecache;
    ASSERT_EQ(tilecache.getNumTiles(), 0);
    ASSERT_EQ(tilecache.getTile(0).has_value(), false );
}

TEST( tilecache, add_image ) {
    TileCache tilecache;
}

TEST( tilecache, add_smt ) {
    TileCache tilecache;
}

TEST( tilecache, add_smf ) {
    TileCache tilecache;
}