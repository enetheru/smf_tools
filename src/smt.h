#pragma once

#include <cstdint>

#include <OpenImageIO/imagebuf.h>

// Borrowing from OpenGL for texture compression formats for implementation
#define GL_UNSIGNED_BYTE                  0x1401
#define GL_UNSIGNED_SHORT                 0x1403

#define GL_R3_G3_B2				0x2A10
#define GL_RGB4					0x804F
#define GL_RGB5					0x8050
#define GL_RGB8					0x8051
#define GL_RGB10				0x8052
#define GL_RGB12				0x8053
#define GL_RGB16				0x8054
#define GL_RGBA2				0x8055
#define GL_RGBA4				0x8056
#define GL_RGB5_A1				0x8057
#define GL_RGBA8				0x8058
#define GL_RGB10_A2				0x8059
#define GL_RGBA12				0x805A
#define GL_RGBA16				0x805B

#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT   0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT  0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT  0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT  0x83F3

#define GL_COMPRESSED_RGB8_ETC2           0x9274
#define GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 0x9276
#define GL_COMPRESSED_RGBA8_ETC2_EAC      0x9278

#define GL_COMPRESSED_RGBA_ASTC_4x4_KHR   0x93B0
#define GL_COMPRESSED_RGBA_ASTC_5x4_KHR   0x93B1
#define GL_COMPRESSED_RGBA_ASTC_5x5_KHR   0x93B2
#define GL_COMPRESSED_RGBA_ASTC_6x5_KHR   0x93B3
#define GL_COMPRESSED_RGBA_ASTC_6x6_KHR   0x93B4
#define GL_COMPRESSED_RGBA_ASTC_8x5_KHR   0x93B5
#define GL_COMPRESSED_RGBA_ASTC_8x6_KHR   0x93B6
#define GL_COMPRESSED_RGBA_ASTC_8x8_KHR   0x93B7
#define GL_COMPRESSED_RGBA_ASTC_10x5_KHR  0x93B8
#define GL_COMPRESSED_RGBA_ASTC_10x6_KHR  0x93B9
#define GL_COMPRESSED_RGBA_ASTC_10x8_KHR  0x93BA
#define GL_COMPRESSED_RGBA_ASTC_10x10_KHR 0x93BB
#define GL_COMPRESSED_RGBA_ASTC_12x10_KHR 0x93BC
#define GL_COMPRESSED_RGBA_ASTC_12x12_KHR 0x93BD


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
    
    void appendDXT1( OpenImageIO::ImageBuf * );
    void appendRGBA8( OpenImageIO::ImageBuf * );
    void appendUSHORT( OpenImageIO::ImageBuf * );
    OpenImageIO::ImageBuf *getTileDXT1( uint32_t );
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

    OpenImageIO::ImageBuf *getTile( uint32_t );
    void append( OpenImageIO::ImageBuf * );
};
