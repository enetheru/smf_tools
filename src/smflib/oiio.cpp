//
// Created by nicho on 17/11/2023.
//
#include "oiio.h"

#include <spdlog/spdlog.h>

HeightIO::HeightIO()
: _smfPath( "out_height.tif" ),
    _imageSpec({ 0, 0, 129, 129, 0, 0, 0, 1 },
               OIIO::TypeUInt16 )
{ }

void
HeightIO::read() {
    std::ifstream file( _smfPath, std::ios::in | std::ios::binary );
    if( file.fail() ){
        SPDLOG_ERROR( "ifstream has failure, aborting read" );
        return;
    }
    _imageBuffer.reset( _imageSpec );

    file.seekg( _position );
    char * pixelData    = static_cast< char* >(_imageBuffer.localpixels());
    const auto dataSize = static_cast< std::streamsize >(_imageSpec.image_bytes());
    file.read( pixelData , dataSize );
    if( file.fail() ) {
        SPDLOG_ERROR( "Unknown Failure when reading from file" );
    }
    file.close();
}

size_t
HeightIO::write() {
    std::ofstream file( _smfPath, std::ios::binary | std::ios::in | std::ios::out );
    if( file.fail() ){
        SPDLOG_ERROR( "ofstream has failure, aborting write" );
        return 0;
    }
    file.seekp( _position );

    if( !_imageBuffer.initialized() ){
        SPDLOG_WARN( "Height input buffer is not initialised, writing zeroes" );
        constexpr char zero = 0;
        for( uint32_t i = 0; i < _size; ++i )
            file.write( &zero, sizeof( char ) );
        return 0;
    }

    _imageBuffer.read( 0, 0, true, _imageSpec.format );

    // write the data to the smf
    const char* pixelData = static_cast< const char* >(_imageBuffer.localpixels());
    const auto dataSize   = static_cast< std::streamsize >(_imageSpec.image_bytes());
    file.write( pixelData , dataSize );

    file.close();

    return 0;
}

void HeightIO::setWidth( const int width ) {
    _imageSpec.width = width;
}
void HeightIO::setHeight( const int height ) {
    _imageSpec.height = height;
}


