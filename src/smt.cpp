#include <fstream>

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <squish.h>

#include "elog/elog.h"

#include "smt.h"
#include "util.h"

using namespace std;
OIIO_NAMESPACE_USING;

SMT *
SMT::create( string fileName, bool overwrite )
{
    SMT *smt;
    ifstream file( fileName );
    if( file.good() && !overwrite ) return nullptr;

    smt = new SMT;
    smt->_fileName = fileName;
    smt->reset();
    return smt;
}


bool
SMT::test( string fileName )
{
    char magic[ 16 ] = "";
    ifstream file( fileName );
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
SMT::open( string fileName )
{
    SMT *smt;
    if( test( fileName ) ){
        smt = new SMT;
        smt->_fileName = fileName;
        smt->load();
        return smt;
    }
    return nullptr;
}

void
SMT::reset( )
{
    LOG(INFO) << "Resetting " << fileName;
    // Clears content of SMT file and re-writes the header.
    fstream file( fileName, ios::binary | ios::out );

    if(! file.good() ){
        LOG(ERROR) << "Unable to write to " << fileName;
        return;
    }

    //reset tile count
    header.nTiles = 0;
    calcTileBytes();

    // write header
    file.write( (char *)&header, sizeof(SMT::Header) );
    file.flush();
    file.close();
}

void
SMT::setFileName( std::string name)
{
    _fileName = name;
}

void
SMT::setType( uint32_t t )
{
    if( header.tileType == t)return;
    header.tileType = t;
    reset();
}

void
SMT::setTileSize( uint32_t r )
{
    if( header.tileSize == r )return;
    header.tileSize = r;
    reset();
}

void
SMT::calcTileBytes()
{
    _tileBytes = 0;

    int mip = header.tileSize;
    _tileSpec.height = tileSize;
    _tileSpec.width = tileSize;
    /*!
     * Calculate the size of the raw format of dxt1 with 4 mip levels
     * DXT1 consists of 64 bits per 4x4 block of pixels.
     * 32x32, 16x16, 8x8, 4x4
     * 512  + 128  + 32 + 8 = 680
     */
    if( header.tileType == 1 ){
        _tileSpec.nchannels = 4;
        _tileSpec.set_format( TypeDesc::UINT8 );
        for( int i=0; i < 4; ++i ){
            _tileBytes += (mip * mip)/2;
            mip /= 2;
        }
    }
    else if( header.tileType == GL_RGBA8 ){
        _tileSpec.nchannels = 1;
        _tileSpec.set_format( TypeDesc::UINT8 );
        for( int i=0; i < 4; ++i ){
            _tileBytes += (mip * mip);
            mip /= 2;
        }
    }
    else if( header.tileType == GL_UNSIGNED_SHORT ){
        _tileSpec.nchannels = 1;
        _tileSpec.set_format( TypeDesc::UINT16 );
        for( int i=0; i < 4; ++i ){
            _tileBytes += (mip * mip) * 2;
            mip /= 2;
        }
    }
    else {
        LOG( FATAL ) << "Invalid tiletype: " << tileType;
    }
}

void
SMT::load( )
{
    ifstream inFile(fileName, ifstream::in);
    CHECK( inFile.good() ) << "Failed to load: " << fileName;
    inFile.read( (char *)&header, sizeof(SMT::Header) );
    calcTileBytes();

    // do some simple checking of file size vs reported tile numbers
    uint32_t actualBytes = 0;
    uint32_t guessBytes = 0;
    uint32_t guessTiles = 0;
    uint32_t remainderBytes = 0;
    inFile.seekg( 0, std::ios::end );
    actualBytes = (uint32_t)inFile.tellg() - 32;
    inFile.close();
    guessBytes = header.nTiles * tileBytes; // file size - header
    guessTiles = actualBytes / tileBytes;
    remainderBytes = actualBytes % tileBytes;


    if( header.nTiles != guessTiles || actualBytes != guessBytes ) {
        LOG( WARN ) << "Possible Data Issue"
            << "\n\t(" << fileName << ").header.nTiles:\033[40G" << header.nTiles
            << "\n\tFile data bytes:\033[40G" << actualBytes
            << "\n\tBytes per tile:\033[40G" << tileBytes
            << "\n\tGuess Tiles:\033[40G" << guessTiles
            << "\n\tGuess Bytes:\033[40G" << guessBytes
            << "\n\tModulus remainder:\033[40G" << remainderBytes;
    }
}

std::string
SMT::info( )
{
    stringstream ss;
    ss <<  "\tFilename: " << fileName << endl
        << "\tVersion: " << header.version << endl
        << "\tTiles: " << header.nTiles << endl
        << "\tTileSize: " << header.tileSize << "x" << header.tileSize << endl
        << "\tCompression: ";
    if( header.tileType == 1 ) ss << "dxt1";
    else if( header.tileType == GL_RGBA8 ) ss << "UINT8";
    else if( header.tileType == GL_UNSIGNED_SHORT ) ss << "UINT16";
    else {
        ss << "UNKNOWN";
    }
    return ss.str();
}

void
SMT::append( const OpenImageIO::ImageBuf &sourceBuf )
{
	sourceBuf.write( "append_input", "tif" );

    if( tileType == 1                 ) appendDXT1(   sourceBuf );
    if( tileType == GL_RGBA8           ) appendRGBA8( sourceBuf );
    if( tileType == GL_UNSIGNED_SHORT ) appendUSHORT( sourceBuf );
}

//FIXME a customised version of this function will probably be a little faster
void
SMT::appendDXT1( const OpenImageIO::ImageBuf &sourceBuf )
{
	sourceBuf.write( "appendDXI1_input.tif", "tif" );
	std::unique_ptr< OpenImageIO::ImageBuf >
			tempBuf( new ImageBuf( sourceBuf ) );
	tempBuf->write( "copyof_appendDXT1_input.tif", "tif" );

    ImageSpec spec;
    int blocks_size = 0;
	//squish::u8 *blocks = nullptr;
	squish::u8 blocks[1024 * 1024 * 4];

    fstream file(fileName, ios::binary | ios::in | ios::out);
    file.seekp( sizeof(SMT::Header) + tileBytes * header.nTiles );
    for( int i = 0; i < 4; ++i ){
        spec = tempBuf->specmod();

        blocks_size = squish::GetStorageRequirements(
            spec.width, spec.height, squish::kDxt1 );
		LOG( INFO ) << "dxt1 requires " << blocks_size << " bytes";
		//if( blocks == nullptr ) blocks = new squish::u8[ blocks_size ];
        // TODO contemplate giving control of compression options to users
        // kColourRangeFit = faster|poor quality
        // kColourMetricPerceptual = default|default
        // kColourIterativeClusterFit = slow|high quality
        squish::CompressImage( (squish::u8 *)tempBuf->localpixels(),
             32, 32, blocks,
            squish::kDxt1 | squish::kColourRangeFit );

        // Write data to smf
        file.write( (char*)blocks, blocks_size );

        spec.width = spec.width >> 1;
        spec.height = spec.height >> 1;
        tempBuf = fix_scale( std::move( tempBuf ), spec );
    }

	//if( blocks != nullptr ) delete blocks;
    ++header.nTiles;

    file.seekp( 20 );
    file.write( (char *)&(header.nTiles), 4 );

    file.flush();
    file.close();
}

void
SMT::appendRGBA8( const OpenImageIO::ImageBuf &sourceBuf )
{
	std::unique_ptr< OpenImageIO::ImageBuf >
			tempBuf( new OpenImageIO::ImageBuf( sourceBuf ) );

    ImageSpec spec;
    fstream file(fileName, ios::binary | ios::in | ios::out);
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
}

void
SMT::appendUSHORT( const OpenImageIO::ImageBuf &sourceBuf )
{
    return;
}

std::unique_ptr< OpenImageIO::ImageBuf >
SMT::getTile( uint32_t n )
{
    if( tileType == 1                 ) return getTileDXT1( n );
//    if( tileType == GL_RGBA8          ) return getTileRGBA8( n );
//    if( tileType == GL_UNSIGNED_SHORT ) return getTileUSHORT( n );
	std::unique_ptr< OpenImageIO::ImageBuf > temp;
    return temp; 
}

std::unique_ptr< OpenImageIO::ImageBuf >
SMT::getTileDXT1( const uint32_t n )
{
	CHECK( n >= header.nTiles ) << "tile index is out of range";

	std::unique_ptr< char > raw_dxt1a( new char[ tileBytes ] );

    ifstream file( fileName );
    CHECK( file.good() ) << "Failed to open file for reading" ;

	file.seekg( sizeof(SMT::Header) + tileBytes * n );
	file.read( (char *)raw_dxt1a.get(), tileBytes );
    file.close();

	std::unique_ptr< char >
			rgba8888( new char[ header.tileSize * header.tileSize * 4 ] );

	squish::DecompressImage( (squish::u8 *)( rgba8888.get() ),
			header.tileSize, header.tileSize, raw_dxt1a.get(), squish::kDxt1 );

	std::unique_ptr< OpenImageIO::ImageBuf >
		tempBuf( new ImageBuf( fileName + "_" + to_string( n ),
				tileSpec, rgba8888.get() ) );

	std::unique_ptr< OpenImageIO::ImageBuf >
			outBuf( new OpenImageIO::ImageBuf );
	outBuf->copy( *tempBuf );

    return outBuf;
}

ImageBuf *
SMT::getTileRGBA8( uint32_t n )
{
    ImageBuf *tempBuf = nullptr;
    if( n >= header.nTiles ){
        tempBuf = new ImageBuf( tileSpec );
        return tempBuf;
    }

    char *pixelData = nullptr;
    ImageBuf *outBuf = nullptr;

    ifstream file( fileName );
    if( file.good() ){
        pixelData = new char[ tileSpec.image_bytes() ];
        outBuf = new ImageBuf;

        file.seekg( sizeof(SMT::Header) + tileBytes * n );
        file.read( pixelData, tileSpec.image_bytes() );

        tempBuf = new ImageBuf( fileName + "_" + to_string(n), tileSpec, pixelData);

        outBuf->copy(*tempBuf);

        delete tempBuf;
        delete [] pixelData;
    }
    file.close();

    return outBuf;
}

ImageBuf *
SMT::getTileUSHORT( uint32_t n )
{
    return nullptr;
}
