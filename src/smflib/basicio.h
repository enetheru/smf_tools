//
// Created by nicho on 20/11/2023.
//

#ifndef BASICIO_H
#define BASICIO_H

#include "smfiobase.h"

#include <recoil/SMFFormat.h>

namespace smflib {

// height
class BasicHeightIo final : public SMFIOBase {
    std::vector<uint16_t>_data;

public:
    void setSMF(SMF* smf) override;
    void update() override;
    void read( std::ifstream& file ) override;
    size_t write( std::ofstream& file ) override;
    [[nodiscard]] nlohmann::ordered_json json() override;
    void reset() override;
};

// type
class BasicTypeIO final : public SMFIOBase {
    std::vector<char>_data;

public:
    void setSMF(SMF* smf) override;
    void update() override;
    void read( std::ifstream& file ) override;
    size_t write( std::ofstream& file ) override;
    [[nodiscard]] nlohmann::ordered_json json() override;
    void reset() override;
};

// mini
class BasicMiniIO final : public SMFIOBase {
    std::vector<char>_data;

public:
    void setSMF(SMF* smf) override;
    void update() override;
    void read( std::ifstream& file ) override;
    size_t write( std::ofstream& file ) override;
    [[nodiscard]] nlohmann::ordered_json json() override;
    void reset() override;
};

// metal
class BasicMetalIO final : public SMFIOBase {
    std::vector<char>_data;

public:
    void setSMF(SMF* smf) override;
    void update() override;
    void read( std::ifstream& file ) override;
    size_t write( std::ofstream& file ) override;
    [[nodiscard]] nlohmann::ordered_json json() override;
    void reset() override;
};

// grass[optional];
class BasicGrassIO final : public SMFIOBase {
    std::vector<char>_data;

public:
    void setSMF(SMF* smf) override;
    void update() override;
    void read( std::ifstream& file ) override;
    size_t write( std::ofstream& file ) override;
    [[nodiscard]] nlohmann::ordered_json json() override;
    void reset() override;
};

// tilemap
class BasicTileIO final : public SMFIOBase {
    /** Tile Section Header.
     *  See the section in SMFFormat.h
     */
    Recoil::MapTileHeader _header{};

    /** List of SMT files and the number of tiles they contain.
     *  the smt list is the list of {numTiles,'fileName'} parsed using the
     * MapTileHeader information.
     */
    using smt_pair = std::pair<uint32_t, std::string>;
    std::vector<smt_pair> _smtList;

    std::vector<uint32_t>_tiles;

public:
    void setSMF(SMF* smf) override;
    void update() override;
    void read( std::ifstream& file ) override;
    size_t write( std::ofstream& file ) override;
    [[nodiscard]] nlohmann::ordered_json json() override;
    void reset() override;
};

// features
class BasicFeatureIO final : public SMFIOBase {
    /** This is followed by numFeatureType zero terminated strings
     * indicating the names of the features in the map then follow numFeatures
     */
    Recoil::MapFeatureHeader _header{};

    /** List of strings
     */
    std::vector<std::string> _types;

    /** Individual features structure
     */
    std::vector<Recoil::MapFeatureStruct> _features;

public:
    void setSMF(SMF* smf) override;
    void update() override;
    void read( std::ifstream& file ) override;
    size_t write( std::ofstream& file ) override;
    [[nodiscard]] nlohmann::ordered_json json() override;
    void reset() override;
};

}
#endif //BASICIO_H
