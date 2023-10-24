#pragma once

#include <cstring>
#include <climits>
#include <cstdint>
#include <string>
#include <vector>
#include <filesystem>

#include <OpenImageIO/imagebuf.h>

#include "tilemap.h"
#include "recoil/SMFFormat.h"

/** Minimap size is defined by a DXT1 compressed 1024x1024 image with 8 mipmaps.<br>
 * 1024   + 512    + 256   + 128  + 64   + 32  + 16  + 8  + 4\n
 * 524288 + 131072 + 32768 + 8192 + 2048 + 512 + 128 + 32 + 8 = 699048
 */
#define MINIMAP_SIZE 699048

#define SMF_HEADER              0x00000001 //!<
#define SMF_EXTRA_HEADER        0x00000002 //!<
#define SMF_HEIGHT              0x00000004 //!<
#define SMF_TYPE                0x00000008 //!<
#define SMF_MAP_HEADER          0x00000010 //!<
#define SMF_MAP                 0x00000020 //!<
#define SMF_MINI                0x00000040 //!<
#define SMF_METAL               0x00000080 //!<
#define SMF_FEATURES_HEADER     0x00000100 //!<
#define SMF_FEATURES            0x00000200 //!<
#define SMF_GRASS               0x00000400 //!<
#define SMF_ALL                 0xFFFFFFFF //!<

// (&= !) turns the flag off
// (|=  ) turns it on


/*! Spring Map File
 *
 */
class SMF {
    std::filesystem::path _filePath;
    uint32_t _dirtyMask = 0xFFFFFFFF;

    /*! Header struct as it is written on disk */
    SMFHeader _header{"spring map file", 1, 0, 128, 128,
                      8, 8, 32, 10, 256};

    /*! Header Extension.
     *
     * start of every header Extn must look like this, then comes data specific
     * for header type
     */
    struct HeaderExtension {
        int bytes = 0;      //!< size of the header
        int type = 0;       //!< type of the header
        HeaderExtension( )= default;
        HeaderExtension( int i, int j ) : bytes( i ), type( j ){ };
    };
    std::vector< SMF::HeaderExtension * > _headerExtensions;

    /*! grass Header Extn.
     *
     * This extension contains a offset to an unsigned char[mapx/4 * mapy/4] array
     * that defines ground vegetation.
     */
    struct HeaderExtn_Grass: public HeaderExtension
    {
        int ptr = 80; ///< offset to beginning of grass map data.
        HeaderExtn_Grass( ) : HeaderExtension(12, 1 ){ };
    };

    OIIO::ImageSpec _heightSpec;
    OIIO::ImageSpec _typeSpec;

    /*! Tile Section Header.
     *
     * this is followed by numTileFiles file definition where each file definition
     * is an int followed by a zero terminated file name. Each file defines as
     * many tiles the int indicates with the following files starting where the
     * last one ended. So if there is 2 files with 100 tiles each the first
     * defines 0-99 and the second 100-199. After this follows an
     * int[ mapx * texelPerSquare / tileSize * mapy * texelPerSquare / tileSize ]
     * which is indexes to the defined tiles
     */
    struct HeaderTiles
    {
        int nFiles = 0; ///< number of files referenced
        int nTiles = 0; ///< number of tiles total
    };
    HeaderTiles _headerTiles;
    std::vector< std::pair< uint32_t, std::string > > _smtList;

    uint32_t _mapPtr{};         ///< pointer to beginning of the tilemap
    OIIO::ImageSpec _mapSpec;

    OIIO::ImageSpec _miniSpec;
    OIIO::ImageSpec _metalSpec;

    /*! Features Section Header.
     *
     * this is followed by numFeatureType zero terminated strings indicating the
     * names of the features in the map then follow numFeatures
     * MapFeatureStructs
     */
    struct HeaderFeatures
    {
        int nTypes = 0;    ///< number of feature types
        int nFeatures = 0; ///< number of features
    };
    HeaderFeatures _headerFeatures;
    std::vector< std::string > _featureTypes; ///< names of features

    /*! Individual features structure
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
    std::vector< SMF::Feature > _features;

    OIIO::ImageSpec _grassSpec;

    // == Internal Utility Functions ==
    OIIO::ImageBuf *getImage( uint32_t ptr, const OIIO::ImageSpec& spec );
    bool writeImage( uint32_t ptr, const OIIO::ImageSpec& spec,
                     OIIO::ImageBuf *sourceBuf = nullptr );

public:
    SMF( )= default;
    ~SMF();

    void good() const;
    static bool test  ( const std::filesystem::path& filePath );
    static SMF *create( std::filesystem::path filePath, bool overwrite = false );
    static SMF *open  ( std::filesystem::path filePath );

    /*! create info string
     *
     * creates a string with information about the class
     */
    std::string info( );

    /*! Update the file offset pointers
     *
    * This function makes sure that all data offset pointers are pointing to the
    * correct location and should be called whenever changes to the class are
    * made that will effect its values.
    */
    void updatePtrs( );

    /*! Update the Image Specifications
     *
     * calculates the image sizes based off the map size
     */
    void updateSpecs( );

    /*! Read the file structure from disk
     *
     * Populates the class from a file on disk
     */
    void read( );

    /*! Set the filename. */
    //FIXME UNUSED void setFilePath( std::filesystem::path filePath );

    /*! Set Map Size uses spring map units.
    *
    * @param width map width in spring map sizes
    * @param length map length in spring map sizes
    */
    void setSize( int width, int length );

     /*! set the size of the mesh squares
     *
     * This is legacy from old ground drawer code. no longer used.
     * @param size Size of the mesh squares
     */
    //TODO UNUSED void setSquareWidth( int size );

    /*! set the density of the images per square
     *
     * @param size density of pixels per square
     */
    //TODO UNUSED void setSquareTexels( int size );

    /*! setTileSize
     *
     * Sets the square pixel resolution of the tiles location in the file.
     * @param size pixels squared
     */
    void setTileSize( int size );

    /*! Set map Depth
     *
     * Negative values for below sea level, positive values for above sea level.
     * Height map values are influenced by these.
     * @param floor lowest point in the world
     * @param ceiling highest point in the world
     */
    void setDepth( float floor, float ceiling );

    void enableGrass( bool enable = false );

    void addTileFile( std::filesystem::path filePath );

    //TODO UNUSED void clearTileFiles( );

    void addFeature( const std::string& name, float x, float y, float z,
                     float r, float s );

    //TODO void addFeatureDefaults();

    //TODO UNUSED void clearFeatures();

    /// add a csv list of features
    void addFeatures( std::filesystem::path filePath );

    void writeHeader( );
    void writeExtraHeaders();
    void writeHeight  ( OIIO::ImageBuf *buf = nullptr );
    void writeType    ( OIIO::ImageBuf *buf = nullptr );
    void writeTileHeader( );
    void writeMap     ( TileMap *tileMap = nullptr );
    void writeMini    ( OIIO::ImageBuf *buf = nullptr );
    void writeMetal   ( OIIO::ImageBuf *buf = nullptr );
    void writeFeatures();
    // Extra
    void writeGrass   ( OIIO::ImageBuf *buf = nullptr );

    OIIO::ImageBuf *getHeight();
    OIIO::ImageBuf *getType();
    std::vector< std::pair< uint32_t, std::string > >
            getSMTList(){ return _smtList; };
    TileMap *getMap();
    OIIO::ImageBuf *getMini();
    OIIO::ImageBuf *getMetal();
    std::string getFeatureTypes();
    std::string getFeatures();
    OIIO::ImageBuf *getGrass();
};
