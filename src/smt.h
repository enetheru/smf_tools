#ifndef __SMT_H
#define __SMT_H

#include <OpenImageIO/imagebuf.h>
#include <cstdint>

class SMT {
public:
    enum TileType { UNKNOWN, DXT1 };
    /*! Header Structure as written on disk.
     */
    struct Header {
        char magic[16] = "spring tilefile";   // "spring tilefile\0"
        uint32_t version = 1;      // must be 1 for now
        uint32_t nTiles = 0;       // total number of tiles in this file
        uint32_t tileSize = 32;      // x and y dimension of tiles, must remain 32 for now.
        uint32_t tileType = TileType::DXT1;     // must be 1=dxt1 for now
    };

private:
    bool init = false;

    Header header;
    uint32_t tileBytes = 680;

    // Input Files
    std::string fileName = "output.smt";
    void calcTileBytes();
    void load();
    
public:
    bool dxt1_quality = false;

    SMT( ){ };
    SMT( std::string f, bool d = false )
        : fileName( f ), dxt1_quality( d ){
        load();
    };

    static SMT *create( std::string fileName,
            bool overwrite = false,
            bool dxt1_quality = false );

    static SMT *open( std::string fileName,
            bool dxt1_quality = false);

    bool initialised( ){ return init; };
    void reset( );
    std::string info();

    void setTileSize( uint32_t r      );
    void setType    ( TileType t ); // 1=DXT1

    uint32_t getTileType ( ){ return header.tileType; };
    uint32_t getTileSize ( ){ return header.tileSize; };
    uint32_t getNTiles   ( ){ return header.nTiles;   };
    uint32_t getTileBytes( ){ return tileBytes;       };
    std::string getFileName( ){ return fileName; };

    OpenImageIO::ImageBuf *getTile( uint32_t tile );
    void append( OpenImageIO::ImageBuf * );
};

#endif //ndef __SMT_H
