#include <fstream>
#include <sstream>
#include <filesystem>

#include <OpenImageIO/imageio.h>
#include <squish/squish.h>
#include <spdlog/spdlog.h>

#include <nlohmann/json.hpp>
#include <utility>

#include "smf.h"
#include "smt.h"
#include "filemap.h"
#include "util.h"

namespace smflib {

bool SMF::checkPath( const std::filesystem::path& filePath )
{
    if(! exists(filePath) ) return false;
    char magic[ 16 ]{};
    if( std::ifstream file( filePath ); file.good() ){
        file.read( magic, 16 );
        file.close();
    }
    return strcmp( magic, "spring map file" ) ? false : true;
}

std::shared_ptr<SMF>
SMF::create( const std::filesystem::path& filePath, const bool overwrite )
{
    // check for existing file and whether to overwrite
    if( exists( filePath )  && !overwrite ) {
        SPDLOG_ERROR( "Refusing to overwrite existing SMF: {}", filePath.string() );
        return nullptr;
    }

    SPDLOG_DEBUG( "Creating {}", filePath.string() );

    // attempt to create a new file or overwrite existing
    std::fstream file( filePath, std::ios::binary | std::ios::out );
    if(! file.good() ){
        SPDLOG_ERROR( "Unable to open file for writing: {}", filePath.string() );
        return nullptr;
    }
    file.close();

    const auto smf = std::make_shared<SMF>();
    smf->_filePath = filePath;
    smf->updateSpecs();
    smf->updatePtrs();
    smf->writeHeader();
    return smf;
}

std::shared_ptr<SMF>
SMF::open( const std::filesystem::path& filePath ){
    if( checkPath( filePath ) ){
        SPDLOG_DEBUG( "Opening {}", filePath.string() );

        auto smf        = std::make_shared<SMF>();
        smf->_dirtyMask = 0x00000000;
        smf->_filePath  = filePath;
        smf->read();
        return smf;
    }

    SPDLOG_ERROR( "Cannot open {}", filePath.string() );
    return {};
}

void
SMF::read()
{
    FileMap map;

    SPDLOG_DEBUG( "Reading ", _filePath.string() );
    std::ifstream file( _filePath );
    if(! file.good() )SPDLOG_ERROR( "unable to read: {}", _filePath.string() );

    file.seekg( 0, std::ios::end );
    // add block after the end of the file to test against.
    map.addBlock( file.tellg(), INT_MAX, "eof" );

    // read header structure.
    file.seekg(0);
    file.read( reinterpret_cast< char* >(&_header), sizeof(_header) );
    updateSpecs();

    // for each pointer, make sure they don't overlap with memory space of
    // other data
    map.addBlock(0,80, "header");
    map.addBlock(_header.heightmapPtr, static_cast< std::streamsize >(_heightSpec.image_bytes()), "height" );
    map.addBlock(_header.typeMapPtr, static_cast< std::streamsize >(_typeSpec.image_bytes()) , "type");
    map.addBlock(_header.minimapPtr, MINIMAP_SIZE, "mini" );
    map.addBlock(_header.metalmapPtr, static_cast< std::streamsize >(_metalSpec.image_bytes()), "metal" );


    // Extra headers Information
    uint32_t offset;
    for( int i = 0; i < _header.numExtraHeaders; ++i ){
        auto headerStub = std::make_unique<Recoil::ExtraHeader>(0, 0);
        offset = file.tellg();
        file.read(reinterpret_cast< char* >(headerStub.get()), sizeof( *headerStub ) );

        switch( headerStub->type ){
        case MEH_Vegetation: {
            SPDLOG_INFO("Found grass header" );
            auto header = std::make_unique< Recoil::HeaderExtn_Grass>();
            // re-read the header into the appropriate container
            file.seekg( offset );
            file.read( reinterpret_cast< char* >(header.get()), sizeof(*header));
            map.addBlock( offset, header->size, "MEH_Vegetation" );
            map.addBlock( header->ptr, static_cast< std::streamsize >(_grassSpec.image_bytes()), "grass" );
            extraHeaders.push_back( std::move( header ) );
            break;
        }
        default: {
            SPDLOG_WARN("Extra Header({}) has unknown type: {}", i, headerStub->type);
            //move the read head to the end of the header
            file.seekg(offset + headerStub->size );
            map.addBlock(offset, headerStub->size, "MEH_UNKNOWN" );
            extraHeaders.push_back( std::move(headerStub ) ); //save the stub for later
        }
        }
    }

    // Tile index Information
    file.seekg( _header.tilesPtr );
    file.read( reinterpret_cast< char* >(&_mapTileHeader), sizeof( Recoil::MapTileHeader ) );
    map.addBlock( _header.tilesPtr, sizeof( Recoil::MapTileHeader ), "mapHeader" );

    // TileFiles
    offset = file.tellg();
    uint32_t nTiles;
    std::string smtFileName;
    for( int i = 0; i < _mapTileHeader.numTileFiles; ++i){
        file.read( reinterpret_cast< char* >(&nTiles), 4 );
        std::getline( file, smtFileName, '\0' );
        _smtList.emplace_back( nTiles, smtFileName );
    }
    if( _mapTileHeader.numTileFiles ){
        map.addBlock( offset, static_cast< uint32_t >(file.tellg()) - offset, "tileFileList");
    }

    // while were at it, let's get the file offset for the tile map.
    _mapPtr = file.tellg();
    map.addBlock( _mapPtr, static_cast< std::streamsize >(_mapSpec.image_bytes()), "map" );

    // Featurelist information
    file.seekg( _header.featurePtr );
    file.read( reinterpret_cast< char* >(&_mapFeatureheader.numFeatureType), 4 );
    file.read( reinterpret_cast< char* >(&_mapFeatureheader.numFeatures), 4 );

    map.addBlock(_header.featurePtr, sizeof( Recoil::MapFeatureHeader ), "featuresHeader" );

    offset = file.tellg();
    std::string featureName;
    for( int i = 0; i < _mapFeatureheader.numFeatureType; ++i ){
        std::getline( file, featureName, '\0' );
        _featureTypes.push_back( featureName );
    }

    if( _mapFeatureheader.numFeatureType)
        map.addBlock( offset, static_cast< uint32_t >(file.tellg()) - offset, "featureNames" );

    Recoil::MapFeatureStruct feature{};
    for( int i = 0; i < _mapFeatureheader.numFeatures; ++i ){
        file.read( reinterpret_cast< char* >(&feature), sizeof(Recoil::MapFeatureStruct) );
        _features.push_back( feature );
    }

    if( _mapFeatureheader.numFeatures ) {
        std::streamsize size =  static_cast<int>(sizeof(Recoil::MapFeatureStruct))
                                * _mapFeatureheader.numFeatures;
        map.addBlock( file.tellg(), size, "features" );
    }

    file.close();
}

void
SMF::updateSpecs()
{
    SPDLOG_DEBUG( "Updating ImageSpec's" );
    // Set _heightSpec.
    _heightSpec.width = _header.mapx + 1;
    _heightSpec.height = _header.mapy + 1;

    // set _typeSpec
    _typeSpec.width = _header.mapx / 2;
    _typeSpec.height = _header.mapy / 2;

    // set map spec
    _mapSpec.width = _header.mapx * 8 / _header.tilesize;
    _mapSpec.height = _header.mapy * 8 / _header.tilesize;

    // set _metalSpec
    _metalSpec.width = _header.mapx / 2;
    _metalSpec.height = _header.mapy / 2;

    // set _grassSpec
    _grassSpec.width = _header.mapx / 4;
    _grassSpec.height = _header.mapy / 4;
}

void
SMF::updatePtrs()
{
    SPDLOG_DEBUG( "Updating file offset pointers" );

    _header.heightmapPtr = sizeof( _header );

    for( const auto &extraHeader : extraHeaders ) _header.heightmapPtr += extraHeader->size;

    _header.typeMapPtr = _header.heightmapPtr + static_cast< int >(_heightSpec.image_bytes());
    _header.tilesPtr   = _header.typeMapPtr + static_cast< int >(_typeSpec.image_bytes());
    _mapPtr            = _header.tilesPtr + static_cast< int >(sizeof( Recoil::MapTileHeader ));

    for( const auto& name : _smtList | std::views::values ) {
        _mapPtr += static_cast< int >(sizeof( uint32_t ) + name.size() + 1);
    }

    _header.minimapPtr  = static_cast<int>(_mapSpec.image_bytes()) + static_cast< int >(_mapPtr);
    _header.metalmapPtr = _header.minimapPtr + MINIMAP_SIZE;
    _header.featurePtr  = _header.metalmapPtr + static_cast< int >(_metalSpec.image_bytes());

    // eof is used here to help with optional extras like grass
    // find out the expected end of the file
    int eof = _header.featurePtr + static_cast< int >(sizeof( Recoil::MapFeatureHeader ));
    for( const auto& i : _featureTypes ) eof += static_cast< int >(i.size()) + 1;
    eof += static_cast< int >(_features.size()) * static_cast< int >(sizeof( Recoil::MapFeatureStruct ));

    // Optional Headers.
    for( const auto &extraHeader : extraHeaders ){
        switch( extraHeader->type ){
        case MEH_None:{
            SPDLOG_WARN( "Extra Header Type is MEH_None" );
            break;
        }
        case MEH_Vegetation:{
            //FIXME grass at the end of file doesnt account for additional extra headers.
            const auto header = reinterpret_cast< Recoil::HeaderExtn_Grass *>(extraHeader.get() );
            header->ptr       = eof;
            eof               = header->ptr + static_cast< int >(_grassSpec.image_bytes());
            break;
        }
        default:{
            SPDLOG_WARN( "unknown header extn" );
        }
        }
    }
}

void
SMF::setSize( const int width, const int length ) {
    if(_header.mapx == width && _header.mapy == length ) return;
    _header.mapx = width * 64;
    _header.mapy = length * 64;
    _dirtyMask |= ALL;
}

void
SMF::setTileSize( const int size )
{
    if(_header.tilesize == size ) return;
    _header.tilesize = size;
    _dirtyMask |= HEADER;
    //FIXME this also effects the tile map, really need to write more elaborate statements
}

void
SMF::setDepth( const float floor, const float ceiling )
{
    if(_header.minHeight == floor && _header.maxHeight == ceiling ) return;
    _header.minHeight = floor;
    _header.maxHeight = ceiling;
    _dirtyMask |= HEADER;
}

void
SMF::enableGrass( const bool enable )
{
    const Recoil::HeaderExtn_Grass *headerGrass{};

    // get header if it exists.
    for( const auto &extraHeader : extraHeaders ){
        if( extraHeader->type == 1 ){
            headerGrass = reinterpret_cast< Recoil::HeaderExtn_Grass *>( extraHeader.get() );
            break;
        }
    }

    // if we have a header, and we don't want it anymore
    if( headerGrass && !enable ){
        const auto erased = erase_if(
                                    extraHeaders,
                                    [](const auto &header) {
                                        return header->type == MEH_Vegetation; } );
        _header.numExtraHeaders -= static_cast< int >(erased);
        _dirtyMask |= ALL;
        return;
    }

    //if it doesn't exist, and we want it
    if( !headerGrass && enable ) {
        // otherwise we don't have and we want one
        extraHeaders.emplace_back(std::make_unique< Recoil::HeaderExtn_Grass>() );
        ++_header.numExtraHeaders;
        _dirtyMask |= ALL;
    }
}

//TODO create a new function that Sets the map y depth and water level.

void
SMF::addTileFile( const std::filesystem::path& filePath )
{
    _dirtyMask |= MAP;

    const auto smt = std::unique_ptr<SMT>( SMT::open( filePath ) );
    if( !smt ) {
        SPDLOG_ERROR( "Invalid smt file ", filePath.string() );
        return;
    }

    ++_mapTileHeader.numTileFiles;
    _mapTileHeader.numTiles += smt->getNumTiles();

    _smtList.emplace_back( smt->getNumTiles(), filePath.filename().string() );
}

void
SMF::addFeature( const std::string& name, Recoil::MapFeatureStruct feature )
{
    bool match = false;
    for( unsigned int i = 0; i < _featureTypes.size(); ++i ){
        if( name != _featureTypes[ i ] ){
            match = true;
            feature.featureType = static_cast< int >(i);
            break;
        }
    }

    if(! match ){
        feature.featureType = static_cast< int >(_featureTypes.size());
        _featureTypes.push_back( name );
    }

    _features.push_back( feature );

    _mapFeatureheader.numFeatureType = static_cast< int >(_featureTypes.size());
    _mapFeatureheader.numFeatures    = static_cast< int >(_features.size());
    _dirtyMask |= FEATURES;
}

void
SMF::addFeatures( const std::filesystem::path& filePath )
{
    // test the file
    std::fstream file( filePath, std::ifstream::in );
    if(! file.good() ) SPDLOG_ERROR( "addFeatures: Cannot open {}", filePath.string() );

    int n = 0;
    std::string cell;
    std::stringstream line;
    std::vector<std::string> tokens;
    while( getline( file, cell ) ){
        ++n;
        line.str( cell );

        tokens.clear();
        while( getline( line, cell, ',' ) ) {
            tokens.push_back( cell );
        }

        try{
            addFeature( tokens[ 0 ],
                { stoi( tokens[ 1 ] ),
                stof( tokens[ 2 ] ),
                stof( tokens[ 3 ] ),
                stof( tokens[ 4 ] ),
                stof( tokens[ 5 ] ),
                stof( tokens[ 6 ] ) } );
        }
        catch (std::invalid_argument const& ex) {
            SPDLOG_WARN( "addFeatures: {}, skipping invalid line at {}", filePath.string(), n );
            SPDLOG_ERROR( "{}", ex.what() );
        }
    }
    file.close();

    /*    DLOG( INFO )
            << "addFeatures"
            << "\n\tTypes: " << _headerFeatures.nTypes
            << "\n\tTypes: " << _headerFeatures.nFeatures;*/
    SPDLOG_ERROR(
R"(addFeatures"
"    Types: {}"
"    Types: {})",
        _mapFeatureheader.numFeatureType,
        _mapFeatureheader.numFeatures );
    _dirtyMask |= FEATURES;
}

/* FIXME Re-introduce when implementing features again.
void
SMF::addFeatureDefaults()
{
    SPDLOG_WARN( "adding default features clears any existing features" );

    clearFeatures();

    for( int i = 0; i < 16; ++i ){
        _featureTypes.push_back( "TreeType" + std::to_string( i ) );
    }
    _featureTypes.emplace_back("GeoVent");
    _headerFeatures.nTypes = _featureTypes.size();
    _dirtyMask |= SMF_FEATURES;
}*/

void
SMF::writeHeader()
{
    SPDLOG_DEBUG( "Writing headers" );

    _header.mapid = rand();

    std::fstream file( _filePath, std::ios::binary | std::ios::in | std::ios::out );
    if( !file.good() )SPDLOG_ERROR( "Unable to open {} for writing", _filePath.string() );

    file.write( reinterpret_cast< char* >(&_header), sizeof(_header) );
    file.close();

    _dirtyMask &= !HEADER;
}

void
SMF::writeExtraHeaders()
{
    SPDLOG_DEBUG( "Writing Extra Headers" );

    std::fstream file( _filePath, std::ios::binary | std::ios::in | std::ios::out );
    if(! file.good() ){
        SPDLOG_ERROR( "Unable to open {} for writing", _filePath.string() );
        return;
    }

    file.seekp( sizeof( _header ) );
    for( const auto &extraheader : extraHeaders ){
        file.write( reinterpret_cast< char* >(extraheader.get()), extraheader->size );
    }
    file.close();

    _dirtyMask &= !EXTRA_HEADER;
}

//FIXME using true and false for error conditions is a PITA, change it to use something more robust.
bool
SMF::writeImage( const unsigned int ptr, const OIIO::ImageSpec& spec, OIIO::ImageBuf &sourceBuf ) const {
    std::fstream file( _filePath, std::ios::binary | std::ios::in | std::ios::out );
    if(! file.good() ){
        SPDLOG_ERROR( "Unable to open {} for writing", _filePath.string() );
        return true;
    }
    file.seekp( ptr );

    if( !sourceBuf.initialized() ){
        constexpr char zero = 0;
        for( uint32_t i = 0; i < spec.image_bytes(); ++i )
            file.write( &zero, sizeof( char ) );
        return true;
    }

    sourceBuf.read( 0, 0, true, spec.format );
    sourceBuf = channels( sourceBuf, spec );
    sourceBuf = scale( sourceBuf, spec );

    // write the data to the smf
    file.write( static_cast< char * >(sourceBuf.localpixels()), static_cast< std::streamsize >(spec.image_bytes()) );
    file.close();

    return false;
}

void
SMF::writeHeight( OIIO::ImageBuf &sourceBuf )
{
    SPDLOG_DEBUG( "Writing height" );
    _dirtyMask &= !HEIGHT;

    if( writeImage(_header.heightmapPtr, _heightSpec, sourceBuf ) ){
        SPDLOG_WARN( "Wrote blank heightmap" );
    }
}

void
SMF::writeType( OIIO::ImageBuf &sourceBuf )
{
    SPDLOG_DEBUG( "INFO: Writing type" );
    _dirtyMask &= !TYPE;

    if( writeImage(_header.typeMapPtr, _typeSpec, sourceBuf ) ){
        SPDLOG_WARN( "Wrote blank typemap" );
    }
}

//FIXME, the fact that this function can fail, but not notify it caller is annoying.
void
SMF::writeMini( OIIO::ImageBuf &sourceBuf )
{
    SPDLOG_DEBUG( "Writing mini" );
    _dirtyMask &= !MINI;

    std::fstream file( _filePath, std::ios::binary | std::ios::in | std::ios::out );
    if(! file.good() ){
        SPDLOG_ERROR( "Unable to open {} for writing", _filePath.string() );
        return;
    }
    file.seekp( _header.minimapPtr );

    if( !sourceBuf.initialized() ){
        char zero[ MINIMAP_SIZE ] = { 0 };
        file.write( zero, sizeof( zero ) );
        file.close();
        SPDLOG_WARN( "Wrote blank minimap" );
        return;
    }

    sourceBuf.read( 0, 0, true, _miniSpec.format );
    sourceBuf = channels( sourceBuf, _miniSpec );
    sourceBuf = scale( sourceBuf, _miniSpec );

    OIIO::ImageSpec spec;
    int blocks_size = 0;
    squish::u8 *blocks = nullptr;
    for( int i = 0; i < 9; ++i ){
        SPDLOG_DEBUG( "mipmap loop: {}", i );
        spec = sourceBuf.specmod();

        blocks_size = GetStorageRequirements(
                spec.width, spec.height, squish::kDxt1 );

        if( blocks == nullptr ){
            SPDLOG_DEBUG( "allocating space: {}", blocks_size );
            blocks = new squish::u8[ blocks_size ];
        }

        SPDLOG_DEBUG( "compressing to dxt1" );
        CompressImage( static_cast< squish::u8* >(sourceBuf.localpixels()),
                spec.width, spec.height, blocks, squish::kDxt1 );

        // Write data to smf
        SPDLOG_DEBUG( "writing dxt1 mip to file" );
        file.write( reinterpret_cast< char* >(blocks), blocks_size );

        spec.width = spec.width >> 1;
        spec.height = spec.height >> 1;

        SPDLOG_DEBUG( "Scaling to: {}x{}", spec.width, spec.height );
        sourceBuf = scale( sourceBuf, spec );
    }
    delete blocks;
    file.close();
}

/// Write the tile header information to the smf
void
SMF::writeTileHeader()
{
    SPDLOG_DEBUG( "Writing tile reference information" );
    _dirtyMask &= !MAP_HEADER;

    std::fstream file( _filePath, std::ios::binary | std::ios::in | std::ios::out );
    file.seekp( _header.tilesPtr );

    // Tiles Header
    file.write( reinterpret_cast< char* >(&_mapTileHeader), sizeof( Recoil::MapTileHeader ) );

    // SMT Names & numbers
    for( auto [numTiles,fileName] : _smtList ){
        file.write( reinterpret_cast< char* >(&numTiles), sizeof(numTiles) );
        file.write( fileName.c_str(), static_cast< std::streamsize >(fileName.size()) + 1 );
    }
    file.close();
}

// write the tile-map information to the smf
void
SMF::writeMap( TileMap *tileMap )
{
    SPDLOG_DEBUG( "Writing map" );
    _dirtyMask &= !MAP;

    if( tileMap == nullptr ){
        OIIO::ImageBuf blank(_mapSpec );
        writeImage( _mapPtr, _mapSpec, blank );
        SPDLOG_WARN( "Wrote blank map" );
        return;
    }

    std::fstream file(_filePath, std::ios::binary | std::ios::in | std::ios::out);
    file.seekp( _mapPtr );
    file.write( reinterpret_cast< char * >(tileMap->data()), static_cast< std::streamsize >(_mapSpec.image_bytes()) );
    file.close();

}

/// write the metal image to the smf
void
SMF::writeMetal( OIIO::ImageBuf &sourceBuf )
{
    SPDLOG_DEBUG( "Writing metal" );
    _dirtyMask &= !METAL;

    if( writeImage(_header.metalmapPtr, _metalSpec, sourceBuf ) )
        SPDLOG_WARN( "Wrote blank metalmap" );
}

/// write the feature header information to the smf
// FIXME, this function can fail but it does not notify its caller
void
SMF::writeFeatures()
{
    SPDLOG_DEBUG( "Writing features" );
    _dirtyMask &= !FEATURES_HEADER;

    std::fstream file( _filePath, std::ios::binary | std::ios::in | std::ios::out );
    if(! file.good() ){
        SPDLOG_ERROR( "Unable to open {} for writing", _filePath.string() );
        return;
    }
    file.seekp( _header.featurePtr );

    // set the current state
    _mapFeatureheader.numFeatureType = static_cast< int >(_featureTypes.size());
    _mapFeatureheader.numFeatures    = static_cast< int >(_features.size());

    file.write( reinterpret_cast< char* >(&_mapFeatureheader), sizeof( Recoil::MapFeatureHeader ) );
    for( const auto& i : _featureTypes ) file.write( i.c_str(), static_cast< std::streamsize >(i.size()) + 1 );
    for( auto i : _features ) file.write( reinterpret_cast< char* >(&i), sizeof(Recoil::MapFeatureStruct) );

    file.close();
}

// Write the grass image to the smf
void
SMF::writeGrass( OIIO::ImageBuf &sourceBuf ) {
    const auto &header = *std::ranges::find_if( extraHeaders,
    [](const auto &candidate){ return candidate->type == MEH_Vegetation; } );
    if( header ){
        const auto headerGrass = reinterpret_cast< Recoil::HeaderExtn_Grass *>( header.get() );
        SPDLOG_DEBUG( "Writing Grass" );
        if( writeImage( headerGrass->ptr, _grassSpec, sourceBuf ) ){
            SPDLOG_WARN( "wrote blank grass map" );
        }
        _dirtyMask &= !GRASS;
    }
}


bool SMF::write() {
    NullWriter heightWriter;
    // == Write ==
    //smf->writeHeader();
    //smf->writeExtraHeaders();
    //smf->writeHeight( heightBuf );
    //smf->writeHeight(empty);
    heightWriter.write();
    //smf->writeType( typeBuf );
    //smf->writeTileHeader();
    //smf->writeTileMap( &tileMap );
    //smf->writeMini( miniBuf );
    //smf->writeMetal( metalBuf );
    //smf->writeMetal( empty );
    //smf->writeFeatures();
    //smf->writeGrass( grassBuf );
    return true;
}

//FIXME returning nullptr on fail is crap.
OIIO::ImageBuf
SMF::getImage( const unsigned int ptr, const OIIO::ImageSpec& spec ) const {
    std::ifstream file( _filePath, std::ios::in | std::ios::binary );
    if(! file.good() ){
        SPDLOG_ERROR( "Unable to open {} for reading", _filePath.string() );
        return {};
    }

    OIIO::ImageBuf imageBuf( spec );

    file.seekg( ptr );
    file.read( static_cast< char* >(imageBuf.localpixels()), static_cast< std::streamsize >(spec.image_bytes()) );
    file.close();

    return imageBuf;
}

OIIO::ImageBuf
SMF::getHeight( ) const {
    return getImage(_header.heightmapPtr, _heightSpec );
}

OIIO::ImageBuf
SMF::getType( ) const {
    return getImage(_header.typeMapPtr, _typeSpec );
}

TileMap
SMF::getMap( ) const {
    TileMap tileMap( _mapSpec.width, _mapSpec.height );
    std::fstream file( _filePath, std::ios::binary | std::ios::in );
    if(! file.good() ){
        SPDLOG_ERROR( "Unable to open {} for reading", _filePath.string() );
        return tileMap;
    }
    file.seekg( _mapPtr );
    for( int y = 0; y < _mapSpec.height; ++y ) {
        for (int x = 0; x < _mapSpec.width; ++x) {
            uint32_t value;
            file.read(reinterpret_cast< char* >(&value), 4);
            tileMap.setXY( x, y, value);
        }
    }
    return tileMap;
}

OIIO::ImageBuf SMF::getMini() const {
    //TODO consider using openimageio dds reader to get minimap

    std::ifstream file( _filePath );
    if(! file.good() ){
        SPDLOG_ERROR( "Unable to open {} for reading", _filePath.string() );
        return {};
    }

    file.seekg( _header.minimapPtr );
    std::array< char, MINIMAP_SIZE > temp{};
    file.read( temp.data(), MINIMAP_SIZE );
    file.close();

    OIIO::ImageBuf imageBuf( _miniSpec );
    DecompressImage( static_cast< squish::u8* >(imageBuf.localpixels()), 1024, 1024, temp.data(), squish::kDxt1);

    return imageBuf;
}

OIIO::ImageBuf
SMF::getMetal( ) const {
    return getImage(_header.metalmapPtr, _metalSpec );
}

std::string
SMF::getFeatureTypes( ) const {
    std::stringstream list;
    for( const auto& i : _featureTypes ) list << i;
    return list.str();
}

std::string
SMF::getFeatures( )
{
    std::stringstream list;
    list << "NAME,X,Y,Z,ANGLE,SCALE\n";
    for( auto [featureType, xpos, ypos, zpos, rotation, relativeSize] : _features ){
        fmt::print( list, "{},{},{},{},{},{}\n",
            _featureTypes[featureType], xpos, ypos, zpos, rotation, relativeSize );
    }

    return list.str();
}

OIIO::ImageBuf
SMF::getGrass() {
    const auto &header = *std::ranges::find_if( extraHeaders,
    [](const auto &candidate){ return candidate->type == MEH_Vegetation; } );

    if( header ){
        const auto headerGrass = reinterpret_cast< Recoil::HeaderExtn_Grass *>( header.get() );
        return getImage( headerGrass->ptr, _grassSpec );
    }
    return {};
}

nlohmann::ordered_json SMF::json() {
    nlohmann::ordered_json j;
    j["filename"] = _filePath;
    j["filesize"] = std::format( "{}", humanise( file_size( _filePath ) ));
    nlohmann::ordered_json h;
    h["version"] = _header.version;
    h["id"] = _header.mapid;
    h["width"] = std::format( "{} | {}", _header.mapx, _header.mapx / 64 );
    h["height"] = std::format( "{} | {}", _header.mapy, _header.mapy / 64 );
    h["squareSize"] = _header.squareSize;
    h["texelPerSquare"] = _header.texelPerSquare;
    h["tilesize"] = _header.tilesize;
    h["minHeight"] = _header.minHeight;
    h["maxHeight"] = _header.maxHeight;
    h["heightmapPtr"] = std::format( "{} | {}x{}:1 UINT16",  to_hex(_header.heightmapPtr ), _header.mapx+1, _header.mapy+1 );
    h["typeMapPtr"] = std::format( "{} | {}x{}:1 UINT8",  to_hex(_header.typeMapPtr ), _header.mapx, _header.mapy );
    h["tilesPtr"] = std::format( "{} | {}x{}:1 UINT32", to_hex(_header.tilesPtr), _header.mapx * 8 / _header.tilesize, _header.mapy * 8 / _header.tilesize );
    h["minimapPtr"] = std::format( "{} | 1024x1024:4 DXT1",  to_hex(_header.minimapPtr ) );
    h["metalmapPtr"] = std::format( "{} | {}x{}:1 UINT8",  to_hex(_header.metalmapPtr ), _header.mapx, _header.mapy );
    h["featurePtr"] = std::format( "{}",  to_hex(_header.featurePtr ) );
    h["numExtraHeaders"] = _header.numExtraHeaders;
    j["header"] = h;

    //Header Extensions
    j["extraHeaders"] = nlohmann::json::array();
    for( const auto &extraHeader : extraHeaders ){
        nlohmann::ordered_json eh;
        switch( extraHeader->type ){
        case MEH_None:{
            eh["size"] = extraHeader->size;
            eh["type"] = fmt::format("{} - Null Header", extraHeader->type );
        }break;
        case MEH_Vegetation:{
            const auto header = reinterpret_cast< Recoil::HeaderExtn_Grass *>( extraHeader.get() );
            eh["size"]        = extraHeader->size;
            eh["type"]        = fmt::format("{} - MEH_Vegetation", extraHeader->type );
            eh["ptr"]         = to_hex( header->ptr );
        }break;
        default:{
            eh["size"] = extraHeader->size;
            eh["type"] = fmt::format("{} - Unknown", extraHeader->type );
        }
        }
        j["extraHeaders"] += eh;
    }

    // Tileindex Information
    nlohmann::ordered_json ti;
    ti["Num Files"] = _smtList.size();
    ti["Num Tiles"] = _mapTileHeader.numTiles;
    ti["File List"] = nlohmann::json::array();

    for( const auto& [numTiles,fileName] : _smtList ){
        ti["File List"] += { numTiles, fileName };
    }
    j["Tile Index Info"] = ti;

    // Features Information
    nlohmann::ordered_json fi;
    fi["Num Features"] = _mapFeatureheader.numFeatures;
    fi["Num Types"] = _mapFeatureheader.numFeatureType;
    j["Feature Info"] = fi;

    return j;
}

void SMF::setMapSize( int width, int height ) {}
void SMF::setHeight( float lower, float upper ) {}
//File Path setter and getter
const std::filesystem::path& SMF::get_file_path() const { return _filePath; }
void SMF::set_file_path(const std::filesystem::path &file_path) {
  _filePath = file_path;
}

// Image Specifications
const OIIO::ImageSpec& SMF::getHeightSpec() const { return _heightSpec; }
const OIIO::ImageSpec& SMF::getTypeSpec() const { return _typeSpec; }
const OIIO::ImageSpec& SMF::getMapSpec() const { return _mapSpec; }
const OIIO::ImageSpec& SMF::getMiniSpec() const { return _miniSpec; }
const OIIO::ImageSpec& SMF::getMetalSpec() const { return _metalSpec; }
const OIIO::ImageSpec& SMF::getGrassSpec() const { return _grassSpec; }

void SMF::setHeightIO( const std::shared_ptr<SMFIOBase>& height_io ) { _heightIO = height_io; }
void SMF::setTypeIO( const std::shared_ptr<SMFIOBase>& type_io ) { _typeIO = type_io; }
void SMF::setMiniIO( const std::shared_ptr<SMFIOBase>& mini_io ) { _miniIO = mini_io; }
void SMF::setMetalIO( const std::shared_ptr<SMFIOBase>& metal_io ) { _metalIO = metal_io; }
void SMF::setFeaturesIO( const std::shared_ptr<SMFIOBase>& features_io ) { _featuresIO = features_io; }
void SMF::setGrassIO( const std::shared_ptr<SMFIOBase>& grass_io ) { _grassIO = grass_io; }
}