#pragma once

#include <cstdint>

#include <OpenImageIO/imagebuf.h>

// Borrowing from OpenGL for texture compression formats for implementation
#define GL_UNSIGNED_SHORT                 0x1403
#define GL_RGBA8				0x8058

class SMT {
public:
    /*! Header Structure as written on disk.
     */
    struct Header {
        char magic[16] = "spring tilefile";   //!< "spring tilefile\0"
        uint32_t version = 1;      //!< must be 1 for now
        uint32_t nTiles = 0;       //!< total number of tiles in this file
        uint32_t tileSize = 32;    //!< x and y dimension of tiles, must remain 32 for now.
        uint32_t tileType = 1;     //!< must be 1=dxt1 for now
    };

private:
    //! File Header
    Header header;

    //! Input Files
    std::string _fileName = "output.smt";

    void calcTileBytes();
    uint32_t _tileBytes = 680; 
    OpenImageIO::ImageSpec _tileSpec;

    //! load data from fileName
    void load();
    
    void appendDXT1(   const OpenImageIO::ImageBuf & );
    void appendRGBA8(  const OpenImageIO::ImageBuf & );
    void appendUSHORT( const OpenImageIO::ImageBuf & );
	std::unique_ptr< OpenImageIO::ImageBuf> getTileDXT1( const uint32_t );
    OpenImageIO::ImageBuf *getTileRGBA8( uint32_t );
    OpenImageIO::ImageBuf *getTileUSHORT( uint32_t );

public:

    const std::string &fileName = _fileName;
    const uint32_t &nTiles = header.nTiles;
    
    const uint32_t &tileType = header.tileType;
    const uint32_t &tileSize = header.tileSize;
    const uint32_t &tileBytes = _tileBytes;
    const OpenImageIO::ImageSpec &tileSpec = _tileSpec;
    

    SMT( ){ };

    /*! File type test.
     *
     * @param fileName the file to test
     * @return true if file is an spring tile file. and false if anything
     * else
     */
    static bool test  ( std::string fileName );

    /*! Create a new SMT file
     *
     * @param fileName The name of the file to write to disk.
     * @param overwrite if true; clobbers existing file data.
     */
    static SMT *create( std::string fileName, bool overwrite = false );
    static SMT *open  ( std::string fileName );

    void reset( );
    std::string info();

    void setTileSize( uint32_t r );
    void setType    ( uint32_t t ); // 1=DXT1
    void setFileName( std::string name );

	std::unique_ptr< OpenImageIO::ImageBuf > getTile( const uint32_t );
    void append( const OpenImageIO::ImageBuf & );
};
