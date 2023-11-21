//
// Created by nicho on 20/11/2023.

#include "basicio.h"

#include "smf.h"


/** BasicHeightIO::read()
 */
void smflib::BasicHeightIo::read( std::ifstream& file ) {
    // Check validity of position
    if( const auto block = _fileMap->getBlockAt( _position ); block.has_value() ) {
        SPDLOG_WARN("header.heightMapPtr({}) exists within an already mapped block({})", _position, block->name );
    }
    file.seekg( _position );
    _data.resize( _bytes / 2, {} );
    file.read( reinterpret_cast<char *>( _data.data() ), _bytes );
}

/** BasicHeightIO::write() - writes data from
 */
size_t smflib::BasicHeightIo::write() {
    // Double check that we have the data to write, padd with zeroes if its too small
    if( _data.capacity() < _bytes )_data.resize( _bytes, 0 );
    //open file
    std::ofstream file( _smf->get_file_path(), std::ios::binary );
    if( file.fail() ) return 0;
    file.seekp( _position );
    file.write( reinterpret_cast<char *>( _data.data() ), _bytes );
    return _bytes;
}

/** BasicHeightIO::update() - Updates any information based on the smf file.
 */
void smflib::BasicHeightIo::update() {
    _position = _smf->header()->heightmapPtr;
    _bytes = _smf->heightMapBytes();
}

/** BasicHeightIO::reset() - clears any data allocated from reading the file.
 */
void smflib::BasicHeightIo::reset() {
    _data.clear();
    _data.shrink_to_fit();
}

void smflib::BasicHeightIo::setSMF( SMF* smf ) {
    SMFIOBase::setSMF( smf );
    _fileMap = smf->getFileMap();
}

nlohmann::ordered_json smflib::BasicHeightIo::json() {
    nlohmann::ordered_json j;
    return j;
}


void smflib::BasicTypeIO::setSMF( SMF* smf ) {
    SMFIOBase::setSMF( smf );
    _fileMap = smf->getFileMap();
}

/**
 * \brief
 */
void smflib::BasicTypeIO::read( std::ifstream& file ) {
    // Check validity of position
    if( const auto block = _fileMap->getBlockAt( _position ); block.has_value() ) {
        SPDLOG_WARN("header.typeMapPtr({}) exists within an already mapped block({})", _position, block->name );
    }
}
size_t smflib::BasicTypeIO::write() { return 0; }
void smflib::BasicTypeIO::update() {
    _position = _smf->header()->typeMapPtr;
    _bytes = _smf->typeMapBytes();
}
void smflib::BasicTypeIO::reset() {}
nlohmann::ordered_json smflib::BasicTypeIO::json() {
    nlohmann::ordered_json j;
    return j;
}


void smflib::BasicMiniIO::setSMF( SMF* smf ) {
    SMFIOBase::setSMF( smf );
    _fileMap = smf->getFileMap();
}

/**
 * \brief
 */
void smflib::BasicMiniIO::read( std::ifstream& file ) {
    // Check validity of position
    if( const auto block = _fileMap->getBlockAt( _position ); block.has_value() ) {
        SPDLOG_WARN("header.miniMapPtr({}) exists within an already mapped block({})", _position, block->name );
    }
}
size_t smflib::BasicMiniIO::write() { return 0; }
void smflib::BasicMiniIO::update() {
    _position = _smf->header()->minimapPtr;
    _bytes = _smf->miniMapBytes();
}
void smflib::BasicMiniIO::reset() {}
nlohmann::ordered_json smflib::BasicMiniIO::json() {
    nlohmann::ordered_json j;
    return j;
}


void smflib::BasicMetalIO::setSMF( SMF* smf ) {
    SMFIOBase::setSMF( smf );
    _fileMap = smf->getFileMap();
}

/**
 * \brief
 */
void smflib::BasicMetalIO::read( std::ifstream& file ) {
    // Check validity of position
    if( const auto block = _fileMap->getBlockAt( _position ); block.has_value() ) {
        SPDLOG_WARN("header.metalMapPtr({}) exists within an already mapped block({})", _position, block->name );
    }
}
size_t smflib::BasicMetalIO::write() { return 0; }
void smflib::BasicMetalIO::update() {
    _position = _smf->header()->metalmapPtr;
    _bytes = _smf->metalMapBytes();
}
void smflib::BasicMetalIO::reset() {}
nlohmann::ordered_json smflib::BasicMetalIO::json() {
    nlohmann::ordered_json j;
    return j;
}


void smflib::BasicGrassIO::setSMF( SMF* smf ) {
    SMFIOBase::setSMF( smf );
    _fileMap = smf->getFileMap();
}

/**
 * \brief
 */
void smflib::BasicGrassIO::read( std::ifstream& file ) {
    // Check validity of position
    if( const auto block = _fileMap->getBlockAt( _position ); block.has_value() ) {
        SPDLOG_WARN("header.grassMapPtr({}) exists within an already mapped block({})", _position, block->name );
    }
}
size_t smflib::BasicGrassIO::write() { return 0; }
void smflib::BasicGrassIO::update() {
    //FIXME Get position through extra headers if there is some.
    // _position = _smf->header()->grassmapPtr;
    _bytes = _smf->grassMapBytes();

}
void smflib::BasicGrassIO::reset() {}
nlohmann::ordered_json smflib::BasicGrassIO::json() {
    nlohmann::ordered_json j;
    return j;
}


/**
 * \brief
 */
void smflib::BasicTileIO::read( std::ifstream& file ) {
    // Check validity of position
    if( const auto block = _fileMap->getBlockAt( _position ); block.has_value() ) {
        SPDLOG_WARN("header.tileMapPtr({}) exists within an already mapped block({})", _position, block->name );
    }

    file.seekg( _position );
    file.read( reinterpret_cast< char* >(&_header), sizeof( Recoil::MapTileHeader ) );
    if( _header.numTileFiles > 1 ) {
        SPDLOG_WARN( "The number of tile files is greater than one, typical maps dont do this, though its not outside the specification." );

        // FIXME guard against bogus values, a failed read or a corrupt map can make the numTileFiles huge.
        // estimatedBytes = numTileFiles * ( sizeof(int) + (filenameBytes =  minimum(1) + extension(4) + nullTermination(1) );
        int estimatedMinimumBytes = _header.numTileFiles * 10;
        //TODO test against the next pointer instead of EOF
        if( estimatedMinimumBytes > _smf->getFileSize() ) {
            SPDLOG_CRITICAL( "Impossible number of tileFiles, read error, or corrupt file." );
            _header.numTileFiles = 1;
        }
        //FIXME we're goin to temporarily block any number larger than 1;
        _header.numTileFiles = 1;
    }

    _fileMap->addBlock( _position, file.tellg() - _position, "mapHeader" );

    // TileFiles
    if( _header.numTiles > 0 ) {
        const std::streampos startNames = file.tellg();

        for( int i = 0; i < _header.numTileFiles; ++i ) {
            uint32_t nTiles;
            std::string smtFileName;
            file.read( reinterpret_cast< char* >(&nTiles), 4 );
            std::getline( file, smtFileName, '\0' );
            _smtList.emplace_back( nTiles, smtFileName );
            // TODO Check filename against pattern *.smt
        }

        _fileMap->addBlock( startNames, file.tellg() - startNames, "tileFileList" );
    }

    // read the tilemap
    const std::streampos startMap = file.tellg();
    _tiles.resize( _bytes / 4 );
    file.read( reinterpret_cast<char *>(_tiles.data() ), _bytes );

    _fileMap->addBlock( startMap, _bytes - startMap, "map" );
}
size_t smflib::BasicTileIO::write() { return 0; }
void smflib::BasicTileIO::update() {
    _position = _smf->header()->tilesPtr;
    _bytes = _smf->tileMapBytes();
}

void smflib::BasicTileIO::reset() {}

void smflib::BasicTileIO::setSMF( SMF* smf ) {
    _smf = smf;
    _fileMap = smf->getFileMap();
}

nlohmann::ordered_json smflib::BasicTileIO::json() {
    nlohmann::ordered_json j;
    j[ "Num Files" ] = _smtList.size();
    j[ "Num Tiles" ] = _header.numTiles;

    j[ "File List" ] = nlohmann::json::array();
    for( const auto& [ numTiles,fileName ] : _smtList ) {
        j[ "File List" ] += { numTiles, fileName };
    }
    return j;
}


/**
 * \brief
 */
void smflib::BasicFeatureIO::read( std::ifstream& file ) {
    // Check validity of position
    if( const auto block = _fileMap->getBlockAt( _position ); block.has_value() ) {
        SPDLOG_WARN("header.featurePtr({}) exists within an already mapped block({})", _position, block->name );
    }
    // Featurelist information
    file.seekg( _position );

    // Read Header
    file.read( reinterpret_cast<char *>(&_header), sizeof(Recoil::MapFeatureHeader) );
    _fileMap->addBlock( _position, sizeof( Recoil::MapFeatureHeader ), "featuresHeader" );

    if( _header.numFeatureType > 1) {
        //TODO guard against bogus values caused by misread, or corrupt file
        //FIXME temporary fixing file
        _header.numFeatureType = 1;
    }
    if( _header.numFeatures > 1) {
        //TODO guard against bogus values caused by misread, or corrupt file
        //FIXME temporary fixing file
        _header.numFeatures = 1;
    }

    // Read Feature Types
    if( _header.numFeatureType > 0 ) {
        const std::streampos startNames = file.tellg();
        std::string featureName;
        for( int i = 0; i < _header.numFeatureType; ++i ) {
            std::getline( file, featureName, '\0' );
            _types.push_back( featureName );
        }
        _fileMap->addBlock( startNames, file.tellg() - startNames, "featureNames" );
    }

    // Read Feature definitions
    if( _header.numFeatureType > 0 ) {
        const std::streampos startFeatures = file.tellg();

        _features.resize( _header.numFeatures,{} );
        for( int i = 0; i < _header.numFeatures; ++i ) {
            file.read( reinterpret_cast< char* >(&_features[i]), sizeof( Recoil::MapFeatureStruct ) );
        }
        _fileMap->addBlock( startFeatures,file.tellg() - startFeatures, "featureDefs" );
    }
}
size_t smflib::BasicFeatureIO::write() { return 0; }
void smflib::BasicFeatureIO::update() {
    _position = _smf->header()->featurePtr;
    _fileMap = _smf->getFileMap();
}
void smflib::BasicFeatureIO::reset() {}

void smflib::BasicFeatureIO::setSMF( SMF* smf ) {
    SMFIOBase::setSMF( smf );
    _fileMap = smf->getFileMap();
}

nlohmann::ordered_json smflib::BasicFeatureIO::json() {
    nlohmann::ordered_json j;
    j[ "Num Features" ] = _header.numFeatures;
    j[ "Num Types" ]    = _header.numFeatureType;
    return j;
}
