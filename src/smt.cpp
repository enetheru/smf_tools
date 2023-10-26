#include <fstream>

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>
#include <squish/squish.h>
#include <spdlog/spdlog.h>

#include "smt.h"
#include "util.h"

SMT *
SMT::create( const std::filesystem::path& filePath, bool overwrite ) {
    SMT *smt;
    std::ifstream file( filePath );
    if( file.good() && !overwrite ) return nullptr;

    smt = new SMT;
    smt->filePath = filePath;
    smt->reset();
    return smt;
}


bool
SMT::test( const std::filesystem::path& filePath ) {
    char magic[ 16 ] = "";
    std::ifstream file( filePath );
    if( file.good() ){
        file.read( (char *)magic, 16 );
        if(! strcmp( magic, "spring tilefile" ) ){
            file.close();
            return true;
        }
    }
    return false;
}

SMT *
SMT::open( const std::filesystem::path& filePath ) {
    SMT *smt;
    if( test( filePath ) ){
        smt = new SMT;
        smt->filePath = filePath;
        smt->load();
        return smt;
    }
    return nullptr;
}

void
SMT::reset( ) {
    SPDLOG_INFO("Resetting {}", filePath.string() );
    // Clears content of SMT file and re-writes the header.
    std::fstream file( filePath, std::ios::binary | std::ios::out );

    if(! file.good() ){
        SPDLOG_ERROR("Unable to write to {}", filePath.string() );
        return;
    }

    //reset tile count
    header.numTiles = 0;
    calcTileBytes();

    // write header
    file.write( (char *)&header, sizeof(header) );
    file.flush();
    file.close();
}

/* FIXME UNUSED
void
SMT::setFilePath( std::filesystem::path _filePath ) {
    filePath = std::move(_filePath);
}*/

void
SMT::setType( int type ) {
    if( header.compressionType == type )return;
    header.compressionType = SMT_HCT_DXT1;
    reset();
}

void
SMT::setTileSize( int size ) {
    if( header.tileSize == size )return;
    header.tileSize = size;
    reset();
}

void
SMT::calcTileBytes() {
    // reset variables
    _tileBytes = 0;
    int mipSize = getTileSize();

    /*!
     * Calculate the size of the raw format of dxt1 with 4 mip levels
     * DXT1 consists of 64 bits per 4x4 block of pixels.
     * 32x32, 16x16, 8x8, 4x4
     * 512  + 128  + 32 + 8 = 680
     */
    if(header.compressionType == 1 ){
        tileSpec = OIIO::ImageSpec(mipSize, mipSize, 4, OIIO::TypeDesc::UINT8 );
        for( int i=0; i < 4; ++i ){
            _tileBytes += (mipSize * mipSize) / 2;
            mipSize /= 2;
        }
    }
    /* FIXME UNUSED
    else if( header.tileType == GL_RGBA8 ){
        _tileSpec = ImageSpec( tileSize, tileSize, 4, TypeDesc::UINT8 );
        for( int i=0; i < 4; ++i ){
            _tileBytes += (mip * mip);
            mip /= 2;
        }
    }
    else if( header.tileType == GL_UNSIGNED_SHORT ){
        _tileSpec = ImageSpec( tileSize, tileSize, 1, TypeDesc::UINT16 );
        for( int i=0; i < 4; ++i ){
            _tileBytes += (mip * mip) * 2;
            mip /= 2;
        }
    }*/
    else {
        SPDLOG_CRITICAL("Invalid tiletype: {}", getTileType() );
        //FIXME, the function can error but it does not notify the caller.
        return;
    }
}

void
SMT::load( ) {
    std::ifstream inFile(filePath, std::ifstream::in);
    if( inFile.good() ){
        SPDLOG_CRITICAL( "Failed to load: {}", filePath.string() );
        //FIXME the function can error but it does not notify the caller
        return;
    }
    inFile.read( (char *)&header, sizeof(header) );
    calcTileBytes();

    // do some simple checking of file size vs reported tile numbers
    uint32_t actualBytes;
    uint32_t guessBytes;
    uint32_t guessTiles;
    uint32_t remainderBytes;
    inFile.seekg( 0, std::ios::end );
    actualBytes = (uint32_t)inFile.tellg() - 32;
    inFile.close();
    guessBytes = header.numTiles * _tileBytes; // file size - header
    guessTiles = actualBytes / _tileBytes;
    remainderBytes = actualBytes % _tileBytes;


    if( header.numTiles != (int)guessTiles || actualBytes != guessBytes ) {
        SPDLOG_WARN(
                R"(Possible Data Issue
    ({}).header.nTiles:\033[40G{}
    File data bytes:\033[40G{}
    Bytes per tile:\033[40G{}
    Guess Tiles:\033[40G{}
    Guess Bytes:\033[40G{}
    Modulus remainder:\033[40G)",
                filePath.string(), header.numTiles, actualBytes, _tileBytes, guessTiles, guessBytes, remainderBytes );
    }
}

std::string
SMT::info( ) const {
    std::stringstream ss;
    ss << "\tFilename: " << filePath << std::endl
       << "\tVersion: " << header.version << std::endl
       << "\tTiles: " << header.numTiles << std::endl
        << "\tTileSize: " << header.tileSize << "x" << header.tileSize << std::endl
        << "\tCompression: ";
    if(header.compressionType == 1 ) ss << "dxt1";
    else if(header.compressionType == GL_RGBA8 ) ss << "UINT8";
    else if(header.compressionType == GL_UNSIGNED_SHORT ) ss << "UINT16";
    else {
        ss << "UNKNOWN";
    }
    return ss.str();
}

void
SMT::append( const OIIO::ImageBuf &sourceBuf ) {
    //sourceBuf.write( "SMT_append_sourcebuf.tif", "tif" );

    if( getTileType() == SMT_HCT_DXT1 ) appendDXT1( sourceBuf );
    //FIXME UNUSED if( tileType == GL_RGBA8           ) appendRGBA8( sourceBuf );
    //FIXME UNUSED if( tileType == GL_UNSIGNED_SHORT ) appendUSHORT( sourceBuf );
}

void
SMT::appendDXT1( const OIIO::ImageBuf &sourceBuf ) {
    // make a copy of the input buffer for processing
    OIIO::ImageBuf tempBuf( sourceBuf );

    // imagespec will let us work through the image contents
    OIIO::ImageSpec spec;
    // block_size tells us how much memory to allocate per DXT compress cycle
    int blocks_size = 0;
    // blocks is the resulting memory area for DXT1 compressed files
    squish::u8 *blocks = nullptr;

    // open our filestream and get ready to start writing dxt images to it
    std::fstream file(filePath, std::ios::binary | std::ios::in | std::ios::out);
    file.seekp( sizeof(header) + _tileBytes * header.numTiles );

    // loop through the mipmaps
    for( int i = 0; i < 4; ++i ){
#ifdef DEBUG_IMG
        std::stringstream ss;
        ss << "SMT.appendDXT1.mip" << i << ".tif";
        SPDLOG_INFO( "writing mip to file: " << ss.str();
        tempBuf->write( ss.str(), "tif" );
#endif
        spec = tempBuf.specmod();
        SPDLOG_INFO( "mip: {}, size: {}x{}x{}" , i, spec.width, spec.height, spec.nchannels );

        blocks_size = squish::GetStorageRequirements( spec.width, spec.height, squish::kDxt1 );
        SPDLOG_INFO( "dxt1 requires {} bytes", blocks_size );

        // allocate memory the first time, will be re-used in subsequent runs.
        if( blocks == nullptr ) blocks = new squish::u8[ blocks_size ];

        // TODO contemplate giving control of compression options to users
        // kColourRangeFit = faster|poor quality
        // kColourMetricPerceptual = default|default
        // kColourIterativeClusterFit = slow|high quality
        if( !tempBuf.localpixels() ){
            SPDLOG_CRITICAL( "pixel data unavailable" );
            //FIXME the function can error but it does not notify the caller
            return;
        }
        squish::CompressImage( (squish::u8 *)tempBuf.localpixels(),
            spec.width, spec.height, blocks,
            squish::kDxt1 | squish::kColourRangeFit );
        SPDLOG_INFO( "\n{}", image_to_hex( (const uint8_t *)tempBuf.localpixels(), spec.width, spec.height ) );
        SPDLOG_INFO( "\n()", image_to_hex( (const uint8_t *)blocks, spec.width, spec.height, 1 ) );

        // Write data to smf
        SPDLOG_INFO( "writing {} to {}", blocks_size, filePath.string() );
        file.write( (char*)blocks, blocks_size );

        spec = OIIO::ImageSpec(spec.width >> 1, spec.height >> 1, spec.nchannels, spec.format );
        tempBuf = scale(  tempBuf, spec );
    }

    if( blocks != nullptr ) delete blocks;
    ++header.numTiles;

    file.seekp( 20 );
    file.write((char *)&(header.numTiles), 4 );

    file.flush();
    file.close();
}

/* FIXME UNUSED
void
SMT::appendRGBA8( const OIIO::ImageBuf &sourceBuf ) {
    std::unique_ptr< OIIO::ImageBuf >
            tempBuf( new OIIO::ImageBuf( sourceBuf ) );

    ImageSpec spec;
    fstream file(filePath, ios::binary | ios::in | ios::out);
    file.seekp( sizeof(SMT::Header) + tileBytes * header.nTiles );
    for( int i = 0; i < 4; ++i ){
        spec = tempBuf->specmod();

        // Write data to smf
        file.write( (char*)tempBuf->localpixels(), spec.image_bytes() );

        spec.width = spec.width >> 1;
        spec.height = spec.height >> 1;
        tempBuf = fix_scale( std::move( tempBuf ), spec );
    }

    ++header.nTiles;

    file.seekp( 20 );
    file.write( (char *)&(header.nTiles), 4 );

    file.flush();
    file.close();
}*/

/* FIXME UNUSED
void
SMT::appendUSHORT( const OIIO::ImageBuf &sourceBuf ){ }*/

OIIO::ImageBuf
SMT::getTile( int index ) {
    if( getTileType() == SMT_HCT_DXT1 ) return getTileDXT1( index );
//    if( tileType == GL_RGBA8          ) return getTileRGBA8( index );
//    if( tileType == GL_UNSIGNED_SHORT ) return getTileUSHORT( index );
    return {};
}

OIIO::ImageBuf
SMT::getTileDXT1( const int index ) {
    if( index >= header.numTiles) {
        SPDLOG_CRITICAL("tile index:{} is out of range 0-{}", index, header.numTiles );
        return {};
    }

    std::vector<char> raw_dxt1a(_tileBytes);

    std::ifstream file( filePath );
    if(! file.good() ){
        SPDLOG_CRITICAL( "Failed to open file:'{}' for reading", filePath.string() );
        return {};
    }

    file.seekg( (int)sizeof(header) + _tileBytes * index );
    file.read( raw_dxt1a.data(), _tileBytes );
    file.close();

    std::vector<char> rgba8888(header.tileSize * header.tileSize * 4);

    squish::DecompressImage( (squish::u8 *)( rgba8888.data() ),
        (int)header.tileSize, (int)header.tileSize, raw_dxt1a.data(), squish::kDxt1 );

    return { std::format("{}_{}", filePath.string(), index ), tileSpec, rgba8888.data() };
}

/* FIXME UNUSED
ImageBuf *
SMT::getTileRGBA8( uint32_t n ) {
    ImageBuf *tempBuf = nullptr;
    if( n >= header.nTiles ){
        tempBuf = new ImageBuf( tileSpec );
        return tempBuf;
    }

    char *pixelData = nullptr;
    ImageBuf *outBuf = nullptr;

    ifstream file( filePath );
    if( file.good() ){
        pixelData = new char[ tileSpec.image_bytes() ];
        outBuf = new ImageBuf;

        file.seekg( sizeof(SMT::Header) + tileBytes * n );
        file.read( pixelData, tileSpec.image_bytes() );

        tempBuf = new ImageBuf( filePath.string() + "_" + to_string(n), tileSpec, pixelData);

        outBuf->copy(*tempBuf);

        delete tempBuf;
        delete [] pixelData;
    }
    file.close();

    return outBuf;
}*/

/* FIXME UNUSED
ImageBuf *
SMT::getTileUSHORT( uint32_t n ) {
    return nullptr;
}*/
