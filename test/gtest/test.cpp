#include <gtest/gtest.h>
#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <squish/squish.h>
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
TEST( utils, fix_channels ){

    // Create input buffer with one channel.
    OIIO::ImageSpec inputSpec( 32, 32, 1, OIIO::TypeDesc::UINT8 );
    std::unique_ptr< OIIO::ImageBuf > inputBuf( new OIIO::ImageBuf( inputSpec ) );

    //create output buffer
    OIIO::ImageSpec outputSpec( 32, 32, 4, OIIO::TypeDesc::UINT8 );

    // run function
    auto outputBuf = fix_channels( std::move( inputBuf ), outputSpec );

    // Test resutls
    // FIXME, I am unhappy with the thoroughness of the test, the setup...
    ASSERT_EQ( outputBuf->spec().nchannels, 4 );
    OIIO::shutdown();
}

// fix_scale
TEST( utils, fix_scale ){

    // Create input buffer with one channel.
    OIIO::ImageSpec inputSpec( 32, 32, 1, OIIO::TypeDesc::UINT8 );
    std::unique_ptr< OIIO::ImageBuf > inputBuf( new OIIO::ImageBuf( inputSpec ) );

    //create output buffer
    OIIO::ImageSpec outputSpec( 123, 456, 4, OIIO::TypeDesc::UINT8 );

    // run function
    auto outputBuf = fix_scale( std::move( inputBuf ), outputSpec );

    // Test resutls
    // FIXME, I am unhappy with the thoroughness of the test, the setup...
    ASSERT_EQ( outputBuf->spec().width, 123 );
    ASSERT_EQ( outputBuf->spec().height, 456 );

    OIIO::shutdown();
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
