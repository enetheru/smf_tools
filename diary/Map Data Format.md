   
```cpp
/// Size in bytes of the minimap (all 9 mipmap levels) in the .smf  
#define MINIMAP_SIZE 699048  

/// Number of mipmap levels stored in the file  
#define MINIMAP_NUM_MIPMAP 9  
```
## SMF Header

```cpp
struct SMFHeader {  
    char magic[16];      ///< "spring map file\0"  
    int version;         ///< Must be 1 for now  
    int mapid;           ///< Sort of a GUID of the file, just set to a random value when writing a map  
  
    int mapx;            ///< Must be divisible by 128  
    int mapy;            ///< Must be divisible by 128  
    int squareSize;      ///< Distance between vertices. Must be 8  
    int texelPerSquare;  ///< Number of texels per square, must be 8 for now  
    int tilesize;        ///< Number of texels in a tile, must be 32 for now  
    float minHeight;     ///< Height value that 0 in the heightmap corresponds to    
float maxHeight;     ///< Height value that 0xffff in the heightmap corresponds to  
  
    int heightmapPtr;    ///< File offset to elevation data (short int[(mapy+1)*(mapx+1)])  
    int typeMapPtr;      ///< File offset to typedata (unsigned char[mapy/2 * mapx/2])  
    int tilesPtr;        ///< File offset to tile data (see MapTileHeader)  
    int minimapPtr;      ///< File offset to minimap (always 1024*1024 dxt1 compresed data plus 8 mipmap sublevels)  
    int metalmapPtr;     ///< File offset to metalmap (unsigned char[mapx/2 * mapy/2])  
    int featurePtr;      ///< File offset to feature data (see MapFeatureHeader)  
  
    int numExtraHeaders; ///< Numbers of extra headers following main header  
}; 
``` 
  
## Extra Header  - base class
Header for extensions in .smf file  
  
Start of every extra header must look like this, then comes data specific  
for header type.  
```cpp
struct ExtraHeader {  
    int size; ///< Size of extra header  
    int type; ///< Type of extra header  
};  
```

Defined types for extra headers  
```cpp
#define MEH_None 0  
#define MEH_Vegetation 1  
```
### Ground Vegetation - Extra Header

Extension containing a ground vegetation map  
  
This extension contains an offset to an unsigned char[mapx/4 * mapy/4] array  
that defines ground vegetation, if it's missing there is no ground vegetation.  
  
## TileMap
The header at offset SMFHeader.tilesPtr in the .smf  
  
MapTileHeader is followed by numTileFiles file definition where each file  
definition is an int followed by a zero terminated file name. On loading,  
Spring prepends the filename with "maps/" and looks in it's VFS for a file  
with the resulting filename. See TileFileHeader for details.  
  
Each file defines as many tiles the int indicates with the following files  
starting where the last one ended so if there is 2 files with 100 tiles each  
the first defines 0-99 and the second 100-199.  
  
After this follows an `int[mapx*texelPerSquare/tileSize * mapy*texelPerSquare/tileSize]` which are indices to the defined tiles. 
```cpp
struct MapTileHeader  
{  
    int numTileFiles; ///< Number of tile files to read in (usually 1)  
    int numTiles;     ///< Total number of tiles  
};
//Followed by
std::pair<int, cstring> tileFiles[numTileFiles];
int tileMap[ mapx * texelPerSquare / tileSize * mapy * texelPerSquare / tileSize ];
```

## Map Features
Read from smf file at offset `SMFHeader.featurePtr`
  
## Map Features Header
```cpp
struct MapFeatureHeader   
{  
    int numFeatureType;  
    int numFeatures;  
};  
//Followed by:
cstring featureNames[numFeatureType]; //<! Names of features
MapFeatureStruct features[numFeatures]; //<! features in map
```
  
### Map Feature
```cpp
struct MapFeatureStruct  
{  
    int featureType;    ///< Index to one of the strings above  
    float xpos;         ///< X coordinate of the feature  
    float ypos;         ///< Y coordinate of the feature (height)  
    float zpos;         ///< Z coordinate of the feature  
  
    float rotation;     ///< Orientation of this feature (-32768..32767 for full circle)  
    float relativeSize; ///< Not used at the moment keep 1  
};
```  
  
## Spring Tile File (.smt)
  
Each 32x32 tile is dxt1 compressed data with 4 mipmap levels. This takes up  
exactly SMALL_TILE_SIZE (680) bytes per tile (512 + 128 + 32 + 8).  
### File Header  
```cpp
#define SMALL_TILE_SIZE 680

struct TileFileHeader  
{  
    char magic[16];      ///< "spring tilefile\0"  
    int version;         ///< Must be 1 for now  
  
    int numTiles;        ///< Total number of tiles in this file  
    int tileSize;        ///< Must be 32 for now  
    int compressionType; ///< Must be 1 (= dxt1) for now  
};
```

TileFileHeader is followed by the raw data for the tiles. 