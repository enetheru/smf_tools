#pragma once

#include <cstdint>

#include <OpenImageIO/imagebuf.h>

class SMT {
public:
    enum TileType{
        UNKNOWN, // not used
        DXT1,    // supported
        DXT3,    // Not supported by spring
        DXT5,    // Not supported by spring
        UINT8,   // Not supported by spring
        UINT16,  // Not supported by spring
        UINT32,  // Not supported by spring
        RGBA8,   // Not supported by spring
        ASTC,    // Not supported by spring
    };
    /*! Header Structure as written on disk.
     */
    struct Header {
        char magic[16] = "spring tilefile";   //!< "spring tilefile\0"
        uint32_t version = 1;      //!< must be 1 for now
        uint32_t nTiles = 0;       //!< total number of tiles in this file
        uint32_t tileSize = 32;      //!< x and y dimension of tiles, must remain 32 for now.
        uint32_t tileType = TileType::DXT1;     //!< must be 1=dxt1 for now
    };

private:
    //! File Header
    Header header;

    //! Input Files
    std::string _fileName = "output.smt";


    void calcTileBytes();
    //! Tile Bytes as calculated by calcTileBytes()
    uint32_t _tileBytes = 680; 

    //! load data from fileName
    void load();
    
    void appendDXT1( OpenImageIO::ImageBuf * );
    void appendUINT8( OpenImageIO::ImageBuf * );
    void appendUINT16( OpenImageIO::ImageBuf * );
    OpenImageIO::ImageBuf *getTileDXT1( uint32_t );
    OpenImageIO::ImageBuf *getTileUINT8( uint32_t );
    OpenImageIO::ImageBuf *getTileUINT16( uint32_t );

public:

    const std::string &fileName = _fileName;
    const uint32_t &nTiles = header.nTiles;
    
    const uint32_t &tileType = header.tileType;
    const uint32_t &tileSize = header.tileSize;
    const uint32_t &tileBytes = _tileBytes;
    

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
    void setType    ( TileType t ); // 1=DXT1
    void setFileName( std::string name );

    OpenImageIO::ImageBuf *getTile( uint32_t );
    void append( OpenImageIO::ImageBuf * );
};
