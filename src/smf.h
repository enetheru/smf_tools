#pragma once

#include <cstring>
#include <climits>
#include <cstdint>
#include <string>
#include <vector>

#include <OpenImageIO/imagebuf.h>

#include "tilemap.h"

/** Minimap size is defined by a DXT1 compressed 1024x1024 image with 8 mipmaps.<br>
 * 1024   + 512    + 256   + 128  + 64   + 32  + 16  + 8  + 4\n
 * 524288 + 131072 + 32768 + 8192 + 2048 + 512 + 128 + 32 + 8 = 699048
 */
#define MINIMAP_SIZE 699048
#define SMF_HEADER_NONE 0
#define SMF_HEADER_GRASS 1

#define SMF_HEADER      0x00000001 //!<
#define SMF_EXTRAHEADER 0x00000002 //!<
#define SMF_HEIGHT      0x00000004 //!<
#define SMF_TYPE        0x00000008 //!<
#define SMF_MAP         0x00000010 //!<
#define SMF_MINI        0x00000020 //!<
#define SMF_METAL       0x00000040 //!<
#define SMF_FEATURES    0x00000080 //!<


/*! \brief Spring Map File
 *
 * This class corresponds to a file on disk, operations are performed
 * on the file on disk as you do them so take care.
 */
class SMF {
    std::string fileName;
    bool init = false;
    uint32_t dirtyMask = 0xFFFFFFFF;

    /*! \\brief Header struct as it is written on disk
     *
     * This structure is written to disk as is, its 80 bytes long.
     */
    struct Header {
        char magic[ 16 ] = "spring map file"; ///< byte:0 \n "spring map file"
        int version = 1;      //!< byte:16 \n Must be 1 for now
        int id;               //!< byte:20 \n Prevents name clashes with other maps.
        int width = 128;      //!< byte:24 \n Map width * 64
        int length = 128;     //!< byte:28 \n Map length * 64
        int squareWidth = 8;  //!< byte:32 \n Distance between vertices. must be 8
        int squareTexels = 8; //!< byte:36 \n Number of texels per square, must be 8 for now
        int tileSize = 32;    //!< byte:40 \n Number of texels in a tile, must be 32 for now
        float floor = 10;     //!< byte:44 \n Height value that 0 in the heightmap corresponds to
        float ceiling = 256;  //!< byte:48 \n Height value that 0xffff in the heightmap corresponds to

        int heightPtr;        //!< byte:52 \n File offset to elevation data `short int[(mapy+1)*(mapx+1)]`
        int typePtr;          //!< byte:56 \n File offset to typedata `unsigned char[mapy/2 * mapx/2]`
        int tilesPtr;         //!< byte:60 \n File offset to tile data

        int miniPtr;          //!< byte:64 \n File offset to minimap,
                              //!< always 1024*1024 dxt1 compresed data with 9 mipmap sublevels
        int metalPtr;         //!< byte:68 \n File offset to metalmap `unsigned char[mapx/2 * mapy/2]`
        int featuresPtr;      //!< byte:72 \n File offset to feature data

        int nHeaderExtras = 0;///< byte:76 \n Fumbers of extra headers following this header
    };/* byte: 80 */
    Header header;

    /*! \brief Extra Header
     *
     * start of every extra header must look like this, then comes data specific
     * for header type
     */
    struct HeaderExtra {
        int bytes = 0;      //!< size of the header
        int type = 0;       //!< type of the header
        HeaderExtra( ){ };
        HeaderExtra( int i, int j ) : bytes( i ), type( j ){ };
    };
    std::vector<SMF::HeaderExtra *> headerExtras;

    /// Extra grass Headerder
    /** This extension contains a offset to an unsigned char[mapx/4 * mapy/4] array
     * that defines ground vegetation.
     */
    struct HeaderGrass: public HeaderExtra
    {
        int ptr = 80; ///< offset to beginning of grass map data.
        HeaderGrass( ) : HeaderExtra( 12, 1 ){ };
    };

    OpenImageIO::ImageSpec heightSpec;
    OpenImageIO::ImageSpec typeSpec;
    
    /// Tile Section Header
    /** this is followed by numTileFiles file definition where each file definition
     *  is an int followed by a zero terminated file name. Each file defines as
     *  many tiles the int indicates with the following files starting where the
     *  last one ended. So if there is 2 files with 100 tiles each the first
     *  defines 0-99 and the second 100-199. After this followes an
     *  int[ mapx * texelPerSquare / tileSize * mapy * texelPerSquare / tileSize ]
     *  which is indexes to the defined tiles
     */
    struct HeaderTiles
    {
        int nFiles = 0; ///< number of files referenced
        int nTiles = 0; ///< number of tiles total
    };
    HeaderTiles headerTiles;
    
    std::vector< std::string > smtList;      ///< list of smt files references
    std::vector< uint32_t > nTiles; ///< number of tiles in corresponding files from smtList
    uint32_t mapPtr;         ///< pointer to beginning of the tilemap
    OpenImageIO::ImageSpec mapSpec;
    
    OpenImageIO::ImageSpec miniSpec;
    OpenImageIO::ImageSpec metalSpec;
    
    /// Features Section Header
    /** this is followed by numFeatureType zero terminated strings indicating the
     *  names of the features in the map then follow numFeatures
     *  MapFeatureStructs
     */
    struct HeaderFeatures
    {
        int nTypes = 0;    ///< number of feature types
        int nFeatures = 0; ///< number of features
    };
    HeaderFeatures headerFeatures;
    std::vector< std::string > featureTypes; ///< names of features

    /// Individual features structure
    /**
     */
    struct Feature
    {
        int type; ///< index to one of the strings above
        float x;  ///< x position on the map
        float y;  ///< y position on the map
        float z;  ///< z position on the map
        float r;  ///< rotation
        float s;  ///< scale, currently unused.
    };
    std::vector< SMF::Feature > features;

    OpenImageIO::ImageSpec grassSpec;
    
    // == Internal Utility Functions ==

    /// Update the file offset pointers
    /** This function makes sure that all data offset pointers are pointing to the
    * correct location and should be called whenever changes to the class are
    * made that will effect its values.
    */
    void updatePtrs( );
    
    void updateSpecs( );
    void read( );

    OpenImageIO::ImageBuf *getImage( uint32_t ptr, OpenImageIO::ImageSpec spec );
    bool writeImage( uint32_t ptr, OpenImageIO::ImageSpec spec,
            OpenImageIO::ImageBuf *sourceBuf = NULL );

public:
    SMF( ){ };
    ~SMF();

    static bool test  ( std::string fileName );
    static SMF *create( std::string fileName, bool overwrite = false );
    static SMF *open  ( std::string fileName );

    bool initialised( ){ return init; };
    std::string info( );

    /**
    * Set Map Size.
    * A more detail description of setSize
    * @param width map width in spring map sizes
    * @param length map lenght in spring map sizes
    */
    void setSize( int width, int length );
    
    /**
     * Set map Depth
     * A mode detailed description of setDepth
     * @param floor lowest point in the world
     * @param ceiling highest point in the world
     */
    void setDepth( float floor, float ceiling );
    
    /**
     * setTileSize
     * a mode detailed description of setTileSize.
     * @param size description
     */
    void setTileSize( int size );

    bool addTileFile( std::string fileName );
    void addFeature( std::string name,
            float x, float y, float z, float r, float s );
    void addFeatures( std::string fileName );

    void writeHeader( );
    void writeExtraHeaders();
    bool writeHeight  ( OpenImageIO::ImageBuf *buf );
    bool writeType    ( OpenImageIO::ImageBuf *buf );
    bool writeTileHeader( );
    bool writeMap     ( TileMap *tileMap );
    bool writeMini    ( OpenImageIO::ImageBuf *buf );
    bool writeMetal   ( OpenImageIO::ImageBuf *buf );
    bool writeFeaturesHeader();
    bool writeFeatures();
    // Extra
    bool writeGrass   ( OpenImageIO::ImageBuf *buf );

    OpenImageIO::ImageBuf *getHeight();
    OpenImageIO::ImageBuf *getType();
    std::vector< std::string> getTileFileNames(){ return smtList; };
    TileMap *getMap();
    OpenImageIO::ImageBuf *getMini();
    OpenImageIO::ImageBuf *getMetal();
    std::string getFeatureTypes();
    std::string getFeatures();
    OpenImageIO::ImageBuf *getGrass();

};
