#include "../src/util.h"
#include "gtest/gtest.h"
#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <squish.h>
#include <fstream>

TEST( utils, valxval ){
    auto result = valxval( "123x456" );
    ASSERT_EQ( result.first, 123u );
    ASSERT_EQ( result.second, 456u );
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
    OIIO_NAMESPACE_USING;

    // Create input buffer with one channel.
    ImageSpec inputSpec( 32, 32, 1, TypeDesc::UINT8 );
    std::unique_ptr< ImageBuf > inputBuf( new ImageBuf( inputSpec ) );

    //create output buffer
    ImageSpec outputSpec( 32, 32, 4, TypeDesc::UINT8 );

    // run function
    auto outputBuf = fix_channels( std::move( inputBuf ), outputSpec );

    // Test resutls
    // FIXME, I am unhappy with the thoroughness of the test, the setup...
    ASSERT_EQ( outputBuf->spec().nchannels, 4 );
}

// fix_scale
TEST( utils, fix_scale ){
    OIIO_NAMESPACE_USING;

    // Create input buffer with one channel.
    ImageSpec inputSpec( 32, 32, 1, TypeDesc::UINT8 );
    std::unique_ptr< ImageBuf > inputBuf( new ImageBuf( inputSpec ) );

    //create output buffer
    ImageSpec outputSpec( 123, 456, 4, TypeDesc::UINT8 );

    // run function
    auto outputBuf = fix_scale( std::move( inputBuf ), outputSpec );

    // Test resutls
    // FIXME, I am unhappy with the thoroughness of the test, the setup...
    ASSERT_EQ( outputBuf->spec().width, 123 );
    ASSERT_EQ( outputBuf->spec().height, 456 );


}

TEST( squish, DXT1CompressImage_raw_zero )
{
    // fake image 256x256xRGBA8
    uint32_t width = 16;
    uint32_t height = 16;
    uint32_t channels = 4;
    uint8_t data[ width * height * channels];

    int blocks_size = squish::GetStorageRequirements(
                    width, height, squish::kDxt1 );

    squish::u8 *blocks = new squish::u8[ blocks_size ];

    //zero fake image data
    for( uint32_t i = 0; i < width * height * 4; ++i )
        data[i] = 0;
    std::cout << "\n" << image_to_hex( data, width, height );


    squish::CompressImage( (squish::u8 *)data,
            width, height, blocks,
            squish::kDxt1 | squish::kColourRangeFit );
    std::cout << "\n" << image_to_hex( blocks, width, height, 1);
}

TEST( squish, DXT1CompressImage_raw_random )
{
    // fake image 256x256xRGBA8
    uint32_t width = 16;
    uint32_t height = 16;
    uint32_t channels = 4;
    uint8_t data[ width * height * channels];

    int blocks_size = squish::GetStorageRequirements(
                    width, height, squish::kDxt1 );

    squish::u8 *blocks = new squish::u8[ blocks_size ];

    //randomise fake image data
    for( uint32_t i = 0; i < width * height * 4; ++i )
        data[i] = rand();
    std::cout << "\n" << image_to_hex( data, width, height );


    squish::CompressImage( (squish::u8 *)data,
            width, height, blocks,
            squish::kDxt1 | squish::kColourRangeFit );
    std::cout << "\n" << image_to_hex( blocks, width, height, 1);
}

TEST( squish, DXT1CompressImage_ImageBuf_zero )
{
    // fake image 256x256xRGBA8
    OpenImageIO::ImageSpec spec( 16, 16, 4, OpenImageIO::TypeDesc::UINT8 );
    OpenImageIO::ImageBuf buf( spec );

    int blocks_size = squish::GetStorageRequirements(
                    spec.width, spec.height, squish::kDxt1 );

    squish::u8 *blocks = new squish::u8[ blocks_size ];

    OpenImageIO::ImageBufAlgo::zero( buf );
    std::cout << "\n" << image_to_hex( (uint8_t *)buf.localpixels(), spec.width, spec.height );

    squish::CompressImage( (squish::u8 *)buf.localpixels(),
            spec.width, spec.height, blocks,
            squish::kDxt1 | squish::kColourRangeFit );
    std::cout << "\n" << image_to_hex( blocks, spec.width, spec.height, 1);
}

TEST( squish, DXT1CompressImage_ImageBuf_gradient )
{
    // fake image 256x256xRGBA8
    OpenImageIO::ImageSpec spec( 16, 16, 4, OpenImageIO::TypeDesc::UINT8 );
    OpenImageIO::ImageBuf buf( spec );

    int blocks_size = squish::GetStorageRequirements(
                    spec.width, spec.height, squish::kDxt1 );

    squish::u8 *blocks = new squish::u8[ blocks_size ];

    float R[4] = { 1, 0, 0, 1 };
    float G[4] = { 0, 1, 0, 1 };
    float B[4] = { 0, 0, 1, 1 };
    float A[4] = { 0, 0, 0, 1 };
    OpenImageIO::ImageBufAlgo::fill( buf, R, B, G, A );
    std::cout << "\n" << image_to_hex( (uint8_t *)buf.localpixels(), spec.width, spec.height );


    squish::CompressImage( (squish::u8 *)buf.localpixels(),
            spec.width, spec.height, blocks,
            squish::kDxt1 | squish::kColourRangeFit );
    std::cout << "\n" << image_to_hex( blocks, spec.width, spec.height, 1);
}



int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
