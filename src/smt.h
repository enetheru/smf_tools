#pragma once

#include <cstdint>

#include <OpenImageIO/imagebuf.h>

class SMT {
public:
    enum TileType { UNKNOWN, DXT1 };
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
    //! not sure
    bool init = false;

    //! File Header
    Header header;

    //! Input Files
    std::string fileName = "output.smt";

    /*! Calculates the number of bytes in each tile.
     * Calculate the size of the raw format of dxt1 with 4 mip levels
     * DXT1 consists of 64 bits per 4x4 block of pixels.
     * 32x32, 16x16, 8x8, 4x4
     * 512  + 128  + 32 + 8 = 680
     */
    void calcTileBytes();
    //! Tile Bytes as calculated by calcTileBytes()
    uint32_t tileBytes = 680; 

    //! load data from fileName
    void load();

public:

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

    bool initialised( ){ return init; };
    void reset( );
    std::string info();

    void setTileSize( uint32_t r );
    void setType    ( TileType t ); // 1=DXT1

    uint32_t getTileType ( ){ return header.tileType; };
    uint32_t getTileSize ( ){ return header.tileSize; };
    uint32_t getNTiles   ( ){ return header.nTiles;   };
    uint32_t getTileBytes( ){ return tileBytes;       };
    std::string getFileName( ){ return fileName; };

    OpenImageIO::ImageBuf *getTile( uint32_t tile );
    void append( OpenImageIO::ImageBuf * );
};
