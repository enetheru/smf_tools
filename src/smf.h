#ifndef __SMF_H
#define __SMF_H

#include <cstring>
#include <climits>
#include <string>
#include <vector>
using namespace std;

#include <OpenImageIO/imagebuf.h>
OIIO_NAMESPACE_USING

/** Minimap size is defined by a DXT1 compressed 1024x1024 image with 8 mipmaps.<br>
 * 1024   + 512    + 256   + 128  + 64   + 32  + 16  + 8  + 4\n
 * 524288 + 131072 + 32768 + 8192 + 2048 + 512 + 128 + 32 + 8 = 699048
 */
#define MINIMAP_SIZE 699048
#define SMF_HEADER_NONE 0
#define SMF_HEADER_GRASS 1

/// Spring Map File
/** This class corresponds to a file on disk, operations are performed
 *  on the file on disk as you do them so take care.
 */
class SMF {
    /// Header struct as it is written on disk
    /** This structure is written to disk as is, its 80 bytes long.
     */
    struct Header {
        Header( ){ };
        Header( int i, int w, int l, int sw, int st, int tr, int f, int c,
                int hp, int tp, int tp2, int mp, int mp2, int fp, int n )
            : id( i ), width( w ), length( l ), squareWidth( sw ),
            squareTexels( st ), tileRes( tr ), floor( f ), ceiling (c ),
            heightPtr( hp ), typePtr( tp ), tilesPtr( mp ), miniPtr( tp2 ),
            metalPtr( mp2 ), featuresPtr( fp ), nHeaderExtras( n ) { };

        char magic[ 16 ] =
            { 's','p','r','i','n','g',' ','m','a','p',' ','f','i','l','e','\0' };
                              ///< byte:0 \n "spring map file"
        int version = 1;      ///< byte:16 \n Must be 1 for now
        int id;               ///< byte:20 \n Prevents name clashes with other maps.
        int width = 128;      ///< byte:24 \n Map width * 64
        int length = 128;     ///< byte:28 \n Map length * 64
        int squareWidth = 8;  ///< byte:32 \n Distance between vertices. must be 8
        int squareTexels = 8; ///< byte:36 \n Number of texels per square, must be 8 for now
        int tileRes = 32;     ///< byte:40 \n Number of texels in a tile, must be 32 for now
        float floor = 10;     ///< byte:44 \n Height value that 0 in the heightmap corresponds to    
        float ceiling = 256;  ///< byte:48 \n Height value that 0xffff in the heightmap corresponds to

        int heightPtr;        ///< byte:52 \n File offset to elevation data `short int[(mapy+1)*(mapx+1)]`
        int typePtr;          ///< byte:56 \n File offset to typedata `unsigned char[mapy/2 * mapx/2]`
        int tilesPtr;         ///< byte:60 \n File offset to tile data

        int miniPtr;          ///< byte:64 \n File offset to minimap,
                              ///  always 1024*1024 dxt1 compresed data with 9 mipmap sublevels
        int metalPtr;         ///< byte:68 \n File offset to metalmap `unsigned char[mapx/2 * mapy/2]`
        int featuresPtr;      ///< byte:72 \n File offset to feature data
            
        int nHeaderExtras = 0;///< byte:76 \n Fumbers of extra headers following this header
    };/* byte: 80 */

    /// Extra Header
    /** start of every extra header must look like this, then comes data specific
     *  for header type
     */
    struct HeaderExtra {
        int size = 0; ///< size of the header
        int type = 0; ///< type of the header
        HeaderExtra( ){ };
        HeaderExtra( int i, int j ) : size( i ), type( j ){ };
    };

    /// Extra grass Header
    /** This extension contains a offset to an unsigned char[mapx/4 * mapy/4] array
     * that defines ground vegetation.
     */
    struct HeaderGrass: public HeaderExtra
    {
        int ptr = 80; ///< offset to beginning of grass map data.
        HeaderGrass( ) : HeaderExtra( 12, 1 ){ };
    };


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

    bool init = false;
    int dirty = INT_MAX;
    string fileName;

    Header header;
    vector<SMF::HeaderExtra *> headerExtras;

    ImageSpec heightSpec;
    ImageSpec typeSpec;

    HeaderTiles headerTiles;
    vector<string> smtList;      ///< list of smt files references
    vector<unsigned int> nTiles; ///< number of tiles in corresponding files from smtList
    unsigned int mapPtr;         ///< pointer to beginning of the tilemap
    ImageSpec mapSpec;

    ImageSpec miniSpec;
    ImageSpec metalSpec;

    HeaderFeatures headerFeatures;
    vector<string> featureTypes; ///< names of features
    vector<SMF::Feature> features;

    ImageSpec grassSpec;

    void updatePtrs( );
    void updateSpecs( );
    void setDirty( int i );
    int getDirty();

    ImageBuf *getImage( unsigned int ptr, ImageSpec spec );
    bool    writeImage( unsigned int ptr, ImageSpec spec, ImageBuf *sourceBuf = NULL );

public:
    bool verbose = false;  ///< Print info to stdout
    bool quiet = false;    ///< supress errros to stderr
    bool slowcomp = false; ///< whethe to use slow/quality dxt1 algorithm
    bool invert = false;   ///< Whether to invert the values of the height image.

    SMF( ){ };
    SMF( string f, bool v = false, bool q = false )
        : fileName( f ), verbose( v ), quiet( q )
        { };

    static SMF *create( string fileName, bool overwrite = false,
            bool verbose = false, bool quiet = false );

    static SMF *open( string fileName,
            bool verbose = false, bool quiet = false );

    bool read( );
    bool saveAs( string fileName );
    bool initialised( ){ return init; };
    string info( );

    void setSize( int width, int length );
    void setDepth( float floor, float ceiling );
    
    bool addTileFile( string fileName );
    void addFeature( string name, float x, float y, float z, float r, float s );

    bool writeHeaders( );
    bool writeHeight  ( ImageBuf *buf );
    bool writeType    ( ImageBuf *buf );
    bool writeTileHeader( );
    bool writeMap     ( ImageBuf *buf );
    bool writeMini    ( ImageBuf *buf );
    bool writeMetal   ( ImageBuf *buf );
    bool writeFeaturesHeader();
    bool writeFeatures();
    // Extra
    bool writeGrass   ( ImageBuf *buf );

    bool reWrite( );

    ImageBuf *getHeight();
    ImageBuf *getType();
    string    getTileFiles();
    ImageBuf *getMap();
    ImageBuf *getMini();
    ImageBuf *getMetal();
    string    getFeatureTypes();
    string    getFeatures();
    ImageBuf *getGrass();

};
#endif //__SMF_H
