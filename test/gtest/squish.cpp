#include <gtest/gtest.h>
#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <squish/squish.h>
#include <fstream>

#include "../src/util.h"

TEST( squish, DXT1CompressImage_raw_zero )
{
    // fake image 256x256xRGBA8
    int width = 16;
    int height = 16;
    uint32_t channels = 4;
    uint8_t data[ width * height * channels];

    int blocks_size = squish::GetStorageRequirements(
                    width, height, squish::kDxt1 );

    auto *blocks = new squish::u8[ blocks_size ];

    //zero fake image data
    for( int i = 0; i < width * height * 4; ++i ) data[ i ] = 0;
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
    OIIO::ImageSpec spec( 16, 16, 4, OIIO::TypeDesc::UINT8 );
    OIIO::ImageBuf buf( spec );

    int blocks_size = squish::GetStorageRequirements(
                    spec.width, spec.height, squish::kDxt1 );

    squish::u8 *blocks = new squish::u8[ blocks_size ];

    OIIO::ImageBufAlgo::zero( buf );
    std::cout << "\n" << image_to_hex( (uint8_t *)buf.localpixels(), spec.width, spec.height );

    squish::CompressImage( (squish::u8 *)buf.localpixels(),
            spec.width, spec.height, blocks,
            squish::kDxt1 | squish::kColourRangeFit );
    std::cout << "\n" << image_to_hex( blocks, spec.width, spec.height, 1);
    OIIO::shutdown();
}

TEST( squish, DXT1CompressImage_ImageBuf_gradient )
{
    // fake image 256x256xRGBA8
    OIIO::ImageSpec spec( 16, 16, 4, OIIO::TypeDesc::UINT8 );
    OIIO::ImageBuf buf( spec );

    int blocks_size = squish::GetStorageRequirements(
                    spec.width, spec.height, squish::kDxt1 );

    squish::u8 *blocks = new squish::u8[ blocks_size ];

    float R[4] = { 1, 0, 0, 1 };
    float G[4] = { 0, 1, 0, 1 };
    float B[4] = { 0, 0, 1, 1 };
    float A[4] = { 0, 0, 0, 1 };
    OIIO::ImageBufAlgo::fill( buf, R, B, G, A );
    std::cout << "\n" << image_to_hex( (uint8_t *)buf.localpixels(), spec.width, spec.height );


    squish::CompressImage( (squish::u8 *)buf.localpixels(),
            spec.width, spec.height, blocks,
            squish::kDxt1 | squish::kColourRangeFit );
    std::cout << "\n" << image_to_hex( blocks, spec.width, spec.height, 1);
    OIIO::shutdown();
}
