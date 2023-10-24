#pragma once

#include <cstdint>
#include <filesystem>

#include <OpenImageIO/imagebuf.h>

#include <recoil/SMFFormat.h>

// Borrowing from OpenGL for texture compression formats for implementation
#define GL_UNSIGNED_SHORT       0x1403
#define GL_RGBA8				0x8058

#define SMT_HCT_DXT1 1 // dxt1 compressed tiles

class SMT {
private:
    //! File Header
    TileFileHeader header{"spring tilefile", 1, 0, 32, SMT_HCT_DXT1 };

    void calcTileBytes();
    uint32_t _tileBytes = 680; 
    OIIO::ImageSpec tileSpec;

    //! load data from fileName
    void load();
    
    void appendDXT1( const OIIO::ImageBuf & );
    //FIXME UNUSED void appendRGBA8(  const OIIO::ImageBuf & );
    //FIXME UNUSED void appendUSHORT( const OIIO::ImageBuf & );
	OIIO::ImageBuf getTileDXT1( uint32_t );
    //FIXME UNUSED OIIO::ImageBuf *getTileRGBA8( uint32_t );
    //FIXME UNUSED OIIO::ImageBuf *getTileUSHORT( uint32_t );

public:
    //FIXME Remove all these references.
    std::filesystem::path filePath = "output.smt";

    [[nodiscard]] int getNumTiles() const{ return header.numTiles; }
    [[nodiscard]] int getTileSize() const{ return header.tileSize; }
    [[nodiscard]] int getTileType() const{ return header.compressionType; }
    [[nodiscard]] uint32_t getTileBytes() const{ return _tileBytes; }



    /*! File type test.
     *
     * @param fileName the file to test
     * @return true if file is an spring tile file. and false if anything
     * else
     */
    static bool test  ( const std::filesystem::path& filePath );

    /*! Create a new SMT file
     *
     * @param fileName The name of the file to write to disk.
     * @param overwrite if true; clobbers existing file data.
     */
    static SMT *create( const std::filesystem::path& filePath, bool overwrite = false );
    static SMT *open  ( const std::filesystem::path& filePath );

    void reset( );
    [[nodiscard]] std::string info() const;

    void setTileSize( int size );
    void setType    ( int type ); // 1=DXT1
    //FIXME UNUSED void setFilePath( std::filesystem::path _filePath );

	OIIO::ImageBuf getTile( uint32_t );
    void append( const OIIO::ImageBuf & );
};
