#pragma once

#include <bitset>
#include <filesystem>
#include <vector>

#include <nlohmann/json.hpp>

#include "filemap.h"
#include "smfiobase.h"

#include "recoil/SMFFormat.h"

// Additional items defined to help deserialisation
namespace Recoil {
/** HeaderExtn_Grass
 * \brief Specialised struct for Grass Header
 */
struct HeaderExtn_Grass : ExtraHeader {
    int ptr = 80;
    HeaderExtn_Grass() : ExtraHeader(12, 1) {}
};

} // namespace Recoil

namespace smflib {

/**
 * \brief Helper class for reading and writing to SMF files.
 */
class SMF final {

    /** the location of the smf file to use to read or write to.
      */
    using Path = std::filesystem::path;
    Path _filePath;
    std::streamsize _fileSize{};
    std::shared_ptr<FileMap> _fileMap;


    /** enum for bitmasking logic based on components.
     */
public:
    enum SMFComponent {
        NONE            = 0x00000000,
        HEADER          = 1u << 0,
        EXTRA_HEADER    = 1u << 1,
        HEIGHT          = 1u << 2,
        TYPE            = 1u << 3,
        MAP_HEADER      = 1u << 4,
        TILE            = 1u << 5,
        MINI            = 1u << 6,
        METAL           = 1u << 7,
        FEATURES_HEADER = 1u << 8,
        FEATURES        = 1u << 9,
        GRASS           = 1u << 10,
        ALL             = 0xFFFFFFFF
    };
private:

    /** Header struct as it is written on disk
     * \brief Primary SMF Header Struct.
     */
    Recoil::SMFHeader _header{
     .magic = "spring map file",
     .version = 1,
     .mapid = 0,
     .mapx = 128,
     .mapy = 128,
     .squareSize = 8,
     .texelPerSquare = 8,
     .tilesize = 32,
     .minHeight = 1.0f,
     .maxHeight = 128.0f,
     .heightmapPtr = 0,
     .typeMapPtr = 0,
     .tilesPtr = 0,
     .minimapPtr = 0,
     .metalmapPtr = 0,
     .featurePtr = 0,
     .numExtraHeaders = 0
    };

    /** Header Extension.
     *  start of every header Extn must look like this, then comes data specific
     * for header type
     */
    std::vector<std::unique_ptr<Recoil::ExtraHeader>> extraHeaders;

    /** Tile Section Header.
     *  See the section in SMFFormat.h
     */
    //Recoil::MapTileHeader _mapTileHeader{};

    /** List of SMT files and the number of tiles they contain.
     *  the smt list is the list of {numTiles,'fileName'} parsed using the
     * MapTileHeader information.
     */
    //using smt_pair = std::pair<uint32_t, std::string>;
    //std::vector<smt_pair> _smtList;

    /** Tile Map Pointer - A convenience to get directly to the tile map.
     * points to the data directly after the MapTileheader following filename, and
     * is a convenience.
     */
    //std::streampos _mapPtr{}; ///< pointer to beginning of the tilemap

    /** This is followed by numFeatureType zero terminated strings
     * indicating the names of the features in the map then follow numFeatures
     */
    //Recoil::MapFeatureHeader _mapFeatureheader{};

    /**
     */
    //std::vector<std::string> _featureTypes;

    /** Individual features structure
     */
    //std::vector<Recoil::MapFeatureStruct> _features;

    // component IO
    std::shared_ptr<SMFIOBase> _heightIO;
    std::shared_ptr<SMFIOBase> _typeIO;
    std::shared_ptr<SMFIOBase> _miniIO;
    std::shared_ptr<SMFIOBase> _metalIO;
    std::shared_ptr<SMFIOBase> _featureIO;
    std::shared_ptr<SMFIOBase> _grassIO;
    std::shared_ptr<SMFIOBase> _tileIO;

    /** Update the file offset pointers
     * This function makes sure that all data offset pointers are pointing to the
     * correct location and should be called whenever changes to the class are
     * made that will effect its values.
     */
    void updateHeader();
    std::streampos endStage1{};
    void updatePtrs2(); // When Tile Files change

public:

    SMF() : _fileMap( std::make_shared<FileMap>() ) {}

    Path getFilePath(){ return _filePath; }

    Recoil::SMFHeader* header() { return &_header; }

    /*! create info string
     *
     * creates a string with information about the class
     */
    [[nodiscard]] nlohmann::ordered_json json();

    void setMapid( const int mapid ){_header.mapid = mapid;}
    void setMapx( const int mapx ){_header.mapx = mapx;}
    void setMapy( const int mapy ){_header.mapy = mapy;}
    [[deprecated]] void setSquareSize( const int squareSize ){_header.squareSize = squareSize;}
    [[deprecated]] void setTexelPerSquare( const int texelPerSquare ){_header.texelPerSquare = texelPerSquare;}

    /*! setTileSize
     *
     * Sets the square pixel resolution of the tiles location in the file.
     * @param tilesize
     */
    [[deprecated]] void setTilesize( const int tilesize ){_header.tilesize = tilesize;}
    void setMinHeight( const float minHeight ){_header.minHeight = minHeight;}
    void setMaxHeight( const float maxHeight ){_header.maxHeight = maxHeight;}

    // Convenience functions which aggregate some units.
    /** Set Map Size uses spring map units.
     * @param width
     * @param height
     */
    void setMapSize( int width, int height );

    /** Set map Depth
     * Negative values for below sea level, positive values for above sea level.
     * Height map values are influenced by these.
     * @param lower, upper - each set the vertical position of the height map
     * that pixel values of 0x0000 and 0xFFFF correspond to
     */
    void setHeight( float lower, float upper );

    [[nodiscard]] const std::filesystem::path &get_file_path() const;
    void set_file_path(const std::filesystem::path &file_path);
    std::shared_ptr<FileMap> getFileMap(){ return _fileMap; }
    [[nodiscard]] std::streamsize getFileSize() const{ return _fileSize; }

    void setHeightIO(const std::shared_ptr<SMFIOBase> &height_io);
    void setTypeIO(const std::shared_ptr<SMFIOBase> &type_io);
    void setMiniIO(const std::shared_ptr<SMFIOBase> &mini_io);
    void setMetalIO(const std::shared_ptr<SMFIOBase> &metal_io);
    void setGrassIO(const std::shared_ptr<SMFIOBase> &grass_io); //Optional
    void setTileIO(const std::shared_ptr<SMFIOBase> &tile_io);
    void setFeatureIO(const std::shared_ptr<SMFIOBase> &feature_io);

    /** Tilemap related functions
     */
    //std::vector<smt_pair> getSMTList();

    /*! Read the file structure from disk
     * @param components - optionally specify which components to read
    */
    void read( int components = ALL );

    /**
     * \brief
     * @param components - optionally specify which components to write
     */
    void write( uint32_t components = ALL );

    /** Size of the height Map in Bytes
      */
    [[nodiscard]] int heightMapBytes() const {
        // TODO evaluate whether this is more correct, or whether its just old inforamtion:
        // const int& tps      = _header.texelPerSquare;
        // const int& tileSize = _header.tilesize;
        // tileMapBytes   = tps * mapx / tileSize * (tps * mapy / tileSize) * 4; // uint32

        return (_header.mapx + 1) * (_header.mapy + 1) * 2; // sizeof(uint16_t) = 2 bytes
    }

    /**
      * \brief Size of the type map in bytes.
      * \return
      */
    [[nodiscard]] int typeMapBytes() const {
        // ReSharper disable once CppRedundantParentheses
        return (_header.mapx / 2) * (_header.mapy / 2);
    }

    /**
     * \brief
     * \return
     */
    [[nodiscard]] int tileMapBytes() const {
        // ReSharper disable once CppRedundantParentheses
        return (_header.mapx / 4) * (_header.mapy / 4) * 4; // sizeof(uint32_t) == 4 bytes;
    }

    /**
     * \brief
     * \return
     * Minimap size is defined by a DXT1 compressed 1024x1024 image with 8 mipmaps.
     * 1024   + 512    + 256   + 128  + 64   + 32  + 16  + 8  + 4
     * 524288 + 131072 + 32768 + 8192 + 2048 + 512 + 128 + 32 + 8 = 699048
     * SMFFormat.h provides a macro for this MINIMAP_SIZE = 699048
     */
    [[nodiscard]] static int miniMapBytes(){
        return 699048;
    }

    /**
     * \brief
     * \return
     */
    [[nodiscard]] int metalMapBytes() const {
     // ReSharper disable once CppRedundantParentheses
        return (_header.mapx / 2) * (_header.mapy / 2);
    }

    /**
     * \brief
     * \return
     */
    [[nodiscard]] int grassMapBytes() const {
     // ReSharper disable once CppRedundantParentheses
        return (_header.mapx / 4) * (_header.mapy / 4);
    }

};

} // namespace smflib