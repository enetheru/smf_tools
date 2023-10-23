#pragma once

#include <cstdint>
#include <filesystem>

#include <OpenImageIO/imagebuf.h>

// Borrowing from OpenGL for texture compression formats for implementation
#define GL_UNSIGNED_SHORT       0x1403
#define GL_RGBA8				0x8058

class SMT {
public:
    /*! Header Structure as written on disk.
     */
    struct Header {
        [[maybe_unused]] char magic[16] = "spring tilefile";   //!< "spring tilefile\0"
        uint32_t version = 1;                                  //!< must be 1 for now
        uint32_t nTiles = 0;                                   //!< total number of tiles in this file
        uint32_t tileSize = 32;                                //!< x and y dimension of tiles, must remain 32 for now.
        uint32_t tileType = 1;                                 //!< must be 1,  1=dxt1
    };

private:
    //! File Header
    Header header;

    //! Input File


    void calcTileBytes();
    uint32_t _tileBytes = 680; 
    OIIO::ImageSpec _tileSpec;

    //! load data from fileName
    void load();
    
    void appendDXT1(   const OIIO::ImageBuf & );
    void appendRGBA8(  const OIIO::ImageBuf & );
    void appendUSHORT( const OIIO::ImageBuf & );
	std::unique_ptr< OIIO::ImageBuf> getTileDXT1( uint32_t );
    OIIO::ImageBuf *getTileRGBA8( uint32_t );
    OIIO::ImageBuf *getTileUSHORT( uint32_t );

public:
    //FIXME Remove all these references.
    std::filesystem::path filePath = "output.smt";
    const uint32_t &nTiles = header.nTiles;
    
    const uint32_t &tileType = header.tileType;
    const uint32_t &tileSize = header.tileSize;
    const uint32_t &tileBytes = _tileBytes;
    const OIIO::ImageSpec &tileSpec = _tileSpec;
    

    SMT( )= default;

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
    std::string info();

    void setTileSize( uint32_t r );
    void setType    ( uint32_t t ); // 1=DXT1
    void setFilePath( std::filesystem::path _filePath );

	std::unique_ptr< OIIO::ImageBuf > getTile( uint32_t );
    void append( const OIIO::ImageBuf & );
};
