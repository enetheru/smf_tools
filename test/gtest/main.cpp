#include <gtest/gtest.h>
#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>
#include <fstream>

#include "../src/util.h"


TEST( utils, valxval ){
    auto [first, second] = valxval( "123x456" );
    ASSERT_EQ( first, 123u );
    ASSERT_EQ( second, 456u );
}

TEST( utils, expandString ){
    auto result = expandString( "1,2,3-7,2-5" );
    ASSERT_EQ( result[0], 1u );
    ASSERT_EQ( result[1], 2u );
    ASSERT_EQ( result[2], 3u );
    ASSERT_EQ( result[3], 4u );
    ASSERT_EQ( result[4], 5u );
    ASSERT_EQ( result[5], 6u );
    ASSERT_EQ( result[6], 7u );
    ASSERT_EQ( result[7], 2u );
    ASSERT_EQ( result[8], 3u );
    ASSERT_EQ( result[9], 4u );
    ASSERT_EQ( result[10], 5u );
}

TEST( utils, to_hex ){
    ASSERT_STREQ(to_hex(uint32_t(16)).c_str(), "0x00000010" );
}

// fix_channels
// ============
TEST( utils, channels ){
    OIIO::ImageSpec inputSpec( 32, 32, 1, OIIO::TypeDesc::UINT8 );
    OIIO::ImageBuf inputBuf(inputSpec );
    OIIO::ImageSpec outputSpec( 32, 32, 4, OIIO::TypeDesc::UINT8 );

    inputBuf = channels( inputBuf , outputSpec );

    ASSERT_EQ( inputBuf.spec().nchannels, 4 );
}

TEST( utils, scale ){
    OIIO::ImageSpec inputSpec( 32, 32, 1, OIIO::TypeDesc::UINT8 );
    OIIO::ImageBuf inputBuf( inputSpec );

    OIIO::ImageSpec outputSpec( 123, 456, 4, OIIO::TypeDesc::UINT8 );

    inputBuf = scale( inputBuf , outputSpec );

    ASSERT_EQ( inputBuf.spec().width, 123 );
    ASSERT_EQ( inputBuf.spec().height, 456 );
}

int main(int argc, char **argv) {
    spdlog::set_pattern("[%l] %s:%#:%! | %v");
    spdlog::set_level(spdlog::level::trace );
    ::testing::InitGoogleTest(&argc, argv);
    auto test_results = RUN_ALL_TESTS();
    OIIO::shutdown();
    return test_results;
}