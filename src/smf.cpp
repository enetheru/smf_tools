#include <fstream>
#include <sstream>
#include <filesystem>

#include <OpenImageIO/imageio.h>
#include <squish/squish.h>
#include <spdlog/spdlog.h>

#include "smf.h"
#include "smt.h"
#include "util.h"

void
SMF::good() const
{
    if( _dirtyMask & SMF_HEADER          ) spdlog::info( "dirty header");
    if( _dirtyMask & SMF_EXTRA_HEADER    ) spdlog::info( "dirty headerExtn");
    if( _dirtyMask & SMF_HEIGHT          ) spdlog::info( "dirty height");
    if( _dirtyMask & SMF_TYPE            ) spdlog::info( "dirty type");
    if( _dirtyMask & SMF_MAP_HEADER      ) spdlog::info( "dirty map header");
    if( _dirtyMask & SMF_MAP             ) spdlog::info( "dirty map");
    if( _dirtyMask & SMF_MINI            ) spdlog::info( "dirty mini");
    if( _dirtyMask & SMF_METAL           ) spdlog::info( "dirty metal");
    if( _dirtyMask & SMF_FEATURES_HEADER ) spdlog::info( "dirty features header");
    if( _dirtyMask & SMF_FEATURES        ) spdlog::info( "dirty features");
    if( _dirtyMask & SMF_GRASS           ) spdlog::info( "dirty grass");

}

bool
SMF::test( const std::filesystem::path& filePath )
{
    char magic[ 16 ] = "";
    std::ifstream file( filePath.c_str() );
    if( file.good() ){
        file.read( magic, 16 );
        if(! strcmp( magic, "spring map file" ) ){
            file.close();
            return true;
        }
    }
    return false;
}

SMF *
SMF::create( std::filesystem::path filePath, bool overwrite )
{
    SMF *smf;
    std::fstream file;

    // check for existing file and whether to overwrite
    file.open( filePath, std::ios::in );
    if( file.good() && !overwrite ) return nullptr;
    file.close();

    SPDLOG_DEBUG( "Creating {}", filePath );

    // attempt to create a new file or overwrite existing
    file.open( filePath, std::ios::binary | std::ios::out );
    if(! file.good() ){
        spdlog::error( "Unable to write to: {}", filePath.string() );
        return nullptr;
    }
    file.close();

    smf = new SMF;
    smf->_filePath = filePath;
    smf->updateSpecs();
    smf->updatePtrs();
    smf->writeHeader();
    return smf;
}

SMF *
SMF::open( std::filesystem::path filePath ){
    SMF *smf;
    if( test( filePath ) ){
        SPDLOG_DEBUG( "Opening {}", filePath );

        smf = new SMF;
        smf->_dirtyMask = 0x00000000;
        smf->_filePath = filePath;
        smf->read();
        return smf;
    }

    spdlog::error( "Cannot open {}", filePath.string() );
    return nullptr;
}

void
SMF::read()
{
    FileMap map;

    SPDLOG_DEBUG( "Reading ", filePath );
    std::ifstream file( _filePath );
    if(! file.good() )spdlog::error( "unable to read: {}", _filePath.string() );

    file.seekg( 0, std::ios::end );
    // add block after the end of the file to test against.
    map.addBlock( file.tellg(), INT_MAX, "eof" );

    // read header structure.
    file.seekg(0);
    file.read( (char *)&_header, sizeof(_header) );
    updateSpecs();

    // for each pointer, make sure they don't overlap with memory space of
    // other data
    map.addBlock(0,80, "header");
    map.addBlock(_header.heightmapPtr, _heightSpec.image_bytes(), "height" );
    map.addBlock(_header.typeMapPtr, _typeSpec.image_bytes() , "type");
    map.addBlock(_header.minimapPtr, MINIMAP_SIZE, "mini" );
    map.addBlock(_header.metalmapPtr, _metalSpec.image_bytes(), "metal" );


    // Extra headers Information
    uint32_t offset;
    for( int i = 0; i < _header.numExtraHeaders; ++i ){
        auto headerStub = std::make_unique<ExtraHeader>(0, 0);
        offset = file.tellg();
        file.read((char *)headerStub.get(), sizeof( *headerStub ) );

        switch( headerStub->type ){
            case MEH_Vegetation: {
                spdlog::info("Found grass header" );
                auto header = std::make_unique<SMF::HeaderExtn_Grass>();
                // re-read the header into the appropriate container
                file.seekg( offset );
                file.read( (char *)header.get(), sizeof(*header));
                map.addBlock( offset, header->size, "MEH_Vegetation" );
                map.addBlock( header->ptr, _grassSpec.image_bytes(), "grass" );
                extraHeaders.push_back( std::move( header ) );
                break;
            }
            default: {
                spdlog::warn("Extra Header({}) has unknown type: {}", i, headerStub->type);
                //move the read head to the end of the header
                file.seekg(offset + headerStub->size );
                map.addBlock(offset, headerStub->size, "MEH_UNKNOWN" );
                extraHeaders.push_back( std::move(headerStub ) ); //save the stub for later
            }
        }
    }

    // Tile index Information
    file.seekg( _header.tilesPtr );
    file.read( (char *)&_headerTiles, sizeof( SMF::HeaderTiles ) );
    map.addBlock( _header.tilesPtr, sizeof( SMF::HeaderTiles ), "mapHeader" );

    // TileFiles
    offset = file.tellg();
    uint32_t nTiles;
    std::string smtFileName;
    for( int i = 0; i < _headerTiles.nFiles; ++i){
        file.read( (char *)&nTiles, 4 );
        std::getline( file, smtFileName, '\0' );
        _smtList.emplace_back( nTiles, smtFileName );
    }
    if( _headerTiles.nFiles ){
        map.addBlock( offset, uint32_t( file.tellg() ) - offset, "tileFileList");
    }

    // while were at it, let's get the file offset for the tile map.
    _mapPtr = file.tellg();
    map.addBlock( _mapPtr, _mapSpec.image_bytes(), "map" );

    // Featurelist information
    file.seekg( _header.featurePtr );
    file.read( (char *)&_headerFeatures.nTypes, 4 );
    file.read( (char *)&_headerFeatures.nFeatures, 4 );

    map.addBlock(_header.featurePtr, sizeof( SMF::HeaderFeatures ), "featuresHeader" );

    offset = file.tellg();
    std::string featureName;
    for( int i = 0; i < _headerFeatures.nTypes; ++i ){
        std::getline( file, featureName, '\0' );
        _featureTypes.push_back( featureName );
    }

    if( _headerFeatures.nTypes)
        map.addBlock( offset, uint32_t( file.tellg() ) - offset, "featureNames" );

    SMF::Feature feature{};
    for( int i = 0; i < _headerFeatures.nFeatures; ++i ){
        file.read( (char *)&feature, sizeof(SMF::Feature) );
        _features.push_back( feature );
    }

    if( _headerFeatures.nFeatures )
        map.addBlock( file.tellg(), sizeof( SMF::Feature) * _headerFeatures.nFeatures, "features" );

    file.close();
}

//TODO format as json
std::string
SMF::info()
{
    std::stringstream info;
    info << "[File Information]"
         << "\n\tFile Name: " << _filePath
         << "\n\tFile Size: " << to_hex( file_size(_filePath ) )
            << "\n[header]: "
            << "\n\tVersion: " << _header.version
            << "\n\tID:      " << _header.mapid

            << "\n\n\tWidth:          " << _header.mapx
            << " | " << _header.mapx / 64
            << "\n\tLength:         " << _header.mapy
            << " | " << _header.mapy / 64
            << "\n\tSquareSize:     " << _header.squareSize
            << "\n\tTexelPerSquare: " << _header.texelPerSquare
            << "\n\tTileSize:       " << _header.tilesize
            << "\n\tMinHeight:      " << _header.minHeight
            << "\n\tMaxHeight:      " << _header.maxHeight

            << "\n\n\tHeightPtr:   " << to_hex(_header.heightmapPtr) << " "
            << _header.mapx + 1 << "x"
            << _header.mapy + 1 << ":" << 1 << " UINT16"
            << "\n\tTypePtr:     " << to_hex(_header.typeMapPtr) << " "
            << _header.mapx << "x" << _header.mapy << ":" << 1 << " UINT8"
            << "\n\tTilesPtr:    " << to_hex(_header.tilesPtr)
            << "\n\tMapPtr:      " << to_hex(_mapPtr) << " "
            << _header.mapx * 8 / _header.tilesize << "x"
            << _header.mapy * 8 / _header.tilesize << ":" << 1 << " UINT32"
         << "\n\tMiniPtr:     " << to_hex(_header.minimapPtr)
         << " " << 1024 << "x" << 1024 << ":" << 4 << " DXT1"
         << "\n\tMetalPtr:    " << to_hex(_header.metalmapPtr)
         << " " << _header.mapx << "x" << _header.mapy << ":" << 1 << "  UINT8"
         << "\n\tFeaturesPtr: "     << to_hex(_header.featurePtr)
         << "\n  HeaderExtensions: "   << _header.numExtraHeaders
        ;

    //Header Extensions
    for( const auto &extraHeader : extraHeaders ){
        switch( extraHeader->type ){
            case MEH_None:{
                info << "\n    Null Header"
                     << "\n\tsize: " << extraHeader->size
                     << "\n\ttype: " << extraHeader->type;
            }
                break;
            case MEH_Vegetation:{
                auto header = static_cast< HeaderExtn_Grass *>( extraHeader.get() );
                info << "\n    Grass"
                     << "\n\tsize: " << header->size
                     << "\n\ttype: " << header->type
                     << "\n\tptr:  " << to_hex(header->ptr );
            }
                break;
            default:{
                info << "\n    Unknown"
                     << "\n\tsize: " << extraHeader->size
                     << "\n\ttype: " << extraHeader->type;
            }

        }
    }

    // Tileindex Information
    info << "\n  Tile Index Information"
         << "\n\tTile Files:  " << _smtList.size()
         << "\n\tTotal tiles: " << _headerTiles.nTiles;
    for( const auto& [a,b] : _smtList ){
        info << "\n\t    " << b << ":" << b <<  std::endl;
    }

    // Features Information
    info << "\n  Features Information"
         << "\n\tFeatures: " << _headerFeatures.nFeatures
         << "\n\tTypes:    " << _headerFeatures.nTypes;

    return info.str();
}

void
SMF::updateSpecs()
{
    SPDLOG_DEBUG( "Updating ImageSpec's" );
    // Set _heightSpec.
    _heightSpec.width = _header.mapx + 1;
    _heightSpec.height = _header.mapy + 1;
    _heightSpec.nchannels = 1;
    _heightSpec.set_format( OIIO::TypeDesc::UINT16 );

    // set _typeSpec
    _typeSpec.width = _header.mapx / 2;
    _typeSpec.height = _header.mapy / 2;
    _typeSpec.nchannels = 1;
    _typeSpec.set_format( OIIO::TypeDesc::UINT8 );

    // set map spec
    _mapSpec.width = _header.mapx * 8 / _header.tilesize;
    _mapSpec.height = _header.mapy * 8 / _header.tilesize;
    _mapSpec.nchannels = 1;
    _mapSpec.set_format( OIIO::TypeDesc::UINT );

    // set _miniSpec
    _miniSpec.width = 1024;
    _miniSpec.height = 1024;
    _miniSpec.nchannels = 4;
    _miniSpec.set_format( OIIO::TypeDesc::UINT8 );

    // set _metalSpec
    _metalSpec.width = _header.mapx / 2;
    _metalSpec.height = _header.mapy / 2;
    _metalSpec.nchannels = 1;
    _metalSpec.set_format( OIIO::TypeDesc::UINT8 );

    // set _grassSpec
    _grassSpec.width = _header.mapx / 4;
    _grassSpec.height = _header.mapy / 4;
    _grassSpec.nchannels = 1;
    _grassSpec.set_format( OIIO::TypeDesc::UINT8 );
}

void
SMF::updatePtrs()
{
    SPDLOG_DEBUG( "Updating file offset pointers" );

    _header.heightmapPtr = sizeof( _header );

    for( const auto &extraHeader : extraHeaders ) _header.heightmapPtr += extraHeader->size;

    _header.typeMapPtr = _header.heightmapPtr + _heightSpec.image_bytes();
    _header.tilesPtr = _header.typeMapPtr + _typeSpec.image_bytes();
    _mapPtr = _header.tilesPtr + sizeof( SMF::HeaderTiles );

    for( const auto& [index,name] : _smtList ) _mapPtr += (sizeof(uint32_t) + name.size() + 1);

    _header.minimapPtr = _mapPtr + _mapSpec.image_bytes();
    _header.metalmapPtr = _header.minimapPtr + MINIMAP_SIZE;
    _header.featurePtr = _header.metalmapPtr + _metalSpec.image_bytes();

    // eof is used here to help with optional extras like grass
    // find out the expected end of the file
    int eof = _header.featurePtr + sizeof( SMF::HeaderFeatures );
    for( const auto& i : _featureTypes ) eof += i.size() + 1;
    eof += _features.size() * sizeof( SMF::Feature );

    // Optional Headers.
    for( const auto &extraHeader : extraHeaders ){
        switch( extraHeader->type ){
            case MEH_None:{
                spdlog::warn( "Extra Header Type is MEH_None" );
                break;
            }
            case MEH_Vegetation:{
                //FIXME grass at the end of file doesnt account for additional extra headers.
                auto header = static_cast<SMF::HeaderExtn_Grass *>(extraHeader.get() );
                header->ptr = eof;
                eof = header->ptr + _grassSpec.image_bytes();
                break;
            }
            default:{
                spdlog::warn( "unknown header extn" );
            }
        }
    }
}

/*FIXME commented out because it isnt used.
void
SMF::setFilePath( std::filesystem::path filePath ) {
    _filePath = std::move( filePath );
    _dirtyMask |= SMF_ALL;
}*/

void
SMF::setSize( int width, int length ) {
    if(_header.mapx == width && _header.mapy == length ) return;
    _header.mapx = width * 64;
    _header.mapy = length * 64;
    _dirtyMask |= SMF_ALL;
}

void
SMF::setTileSize( int size )
{
    if(_header.tilesize == size ) return;
    _header.tilesize = size;
    _dirtyMask |= SMF_HEADER;
    //FIXME this also effects the tile map, really need to write more elaborate statements
}

void
SMF::setDepth( float floor, float ceiling )
{
    if(_header.minHeight == floor && _header.maxHeight == ceiling ) return;
    _header.minHeight = floor;
    _header.maxHeight = ceiling;
    _dirtyMask |= SMF_HEADER;
}

void
SMF::enableGrass( bool enable )
{
    HeaderExtn_Grass *headerGrass = nullptr;

    // get header if it exists.
    for( const auto &extraHeader : extraHeaders ){
        if( extraHeader->type == 1 ){
            headerGrass = static_cast<HeaderExtn_Grass *>( extraHeader.get() );
            break;
        }
    }

    //if it doesn't exist, and we don't want it, do nothing
    if(! (enable || headerGrass) ) return;

    // if we have a header, and we don't want it anymore

    if( !enable && headerGrass ){
        auto erased = erase_if(
                extraHeaders,
                [](const auto &header) {
                    return header->type == MEH_Vegetation; } );
        _header.numExtraHeaders -= erased;
        _dirtyMask |= SMF_ALL;
        return;
    }

    // otherwise we don't have and we want one
    extraHeaders.emplace_back(std::make_unique<HeaderExtn_Grass>() );
    ++_header.numExtraHeaders;
    _dirtyMask |= SMF_ALL;
}

//TODO create a new function that Sets the map y depth and water level.

void
SMF::addTileFile( std::filesystem::path filePath )
{
    _dirtyMask |= SMF_MAP;

    SMT *smt = SMT::open( filePath.string() );
    if( smt == nullptr ) spdlog::error( "Invalid smt file ", filePath.string() );

    ++_headerTiles.nFiles;
    _headerTiles.nTiles += smt->nTiles;
    
    //FIXME Figure out what this is attempting to do:
    _smtList.emplace_back( smt->nTiles,
            filePath.string().substr( filePath.string().find_last_of( "/\\" ) + 1 ) );
    

    delete smt;
}

/* FIXME commented out because it isnt used.
void
SMF::clearTileFiles()
{
    _smtList.clear();
    _headerTiles.nFiles = 0;
    _headerTiles.nTiles = 0;
}*/

void
SMF::addFeature( const std::string& name, float x, float y, float z, float r, float s )
{
    SMF::Feature feature{};
    feature.x = x; feature.y = y; feature.z = z;
    feature.r = r; feature.z = s;

    bool match = false;
    for( unsigned int i = 0; i < _featureTypes.size(); ++i ){
        if( name != _featureTypes[ i ] ){
            match = true;
            feature.type = i;
            break;
        }
    }

    if(! match ){
        feature.type = _featureTypes.size();
        _featureTypes.push_back( name );
    }

    _features.push_back( feature );

    _headerFeatures.nTypes = _featureTypes.size();
    _headerFeatures.nFeatures = _features.size();
    _dirtyMask |= SMF_FEATURES;
}

void
SMF::addFeatures( std::filesystem::path filePath )
{
    // test the file
    std::fstream file( filePath, std::ifstream::in );
    if(! file.good() ) spdlog::error( "addFeatures: Cannot open {}", filePath.string() );

    int n = 0;
    std::string cell;
    std::stringstream line;
    std::vector<std::string> tokens;
    while( getline( file, cell ) ){
        ++n;
        line.str( cell );

        tokens.clear();
        while( getline( line, cell, ',' ) ) tokens.push_back( cell );
        if( tokens.size() != 6 ) continue;

        try{
            addFeature(
                tokens[ 0 ],           //name
                stof( tokens[ 1 ] ),   //x
                stof( tokens[ 2 ] ),   //y
                stof( tokens[ 3 ] ),   //z
                stof( tokens[ 4 ] ),   //r
                stof( tokens[ 5 ] ) ); //s
        }
        catch (std::invalid_argument const& ex) {
            spdlog::warn( "addFeatures: {}, skipping invalid line at {}", filePath.string(), n );
            spdlog::error( "{}", ex.what() );
            continue;
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
        _headerFeatures.nTypes,
        _headerFeatures.nFeatures );
    _dirtyMask |= SMF_FEATURES;
}

/* FIXME commented out because it isnt used.
void
SMF::addFeatureDefaults()
{
    spdlog::warn( "adding default features clears any existing features" );

    clearFeatures();

    for( int i = 0; i < 16; ++i ){
        _featureTypes.push_back( "TreeType" + std::to_string( i ) );
    }
    _featureTypes.emplace_back("GeoVent");
    _headerFeatures.nTypes = _featureTypes.size();
    _dirtyMask |= SMF_FEATURES;
}*/

/* FIXME commented out because it isnt used.
void
SMF::clearFeatures()
{
    _features.clear();
    _headerFeatures.nFeatures = 0;

    _featureTypes.clear();
    _headerFeatures.nTypes = 0;

    _dirtyMask |= SMF_FEATURES;
}*/

void
SMF::writeHeader()
{
    SPDLOG_DEBUG( "Writing headers" );

    _header.mapid = rand();

    std::fstream file( _filePath, std::ios::binary | std::ios::in | std::ios::out );
    if( !file.good() )spdlog::error( "Unable to open {} for writing", _filePath.string() );

    file.write( (char *)&_header, sizeof(_header) );
    file.close();

    _dirtyMask &= !SMF_HEADER;
}

void
SMF::writeExtraHeaders()
{
    SPDLOG_DEBUG( "Writing Extra Headers" );

    std::fstream file( _filePath, std::ios::binary | std::ios::in | std::ios::out );
    if(! file.good() ){
        spdlog::error( "Unable to open {} for writing", _filePath.string() );
        return;
    }

    file.seekp( sizeof( _header ) );
    for( const auto &extraheader : extraHeaders ){
        file.write( (char *)extraheader.get(), extraheader->size );
    }
    file.close();

    _dirtyMask &= !SMF_EXTRA_HEADER;
}

//FIXME using true and false for error conditions is a PITA, change it to use something more robust.
bool
SMF::writeImage( unsigned int ptr, const OIIO::ImageSpec& spec, OIIO::ImageBuf *sourceBuf )
{
    std::fstream file( _filePath, std::ios::binary | std::ios::in | std::ios::out );
    if(! file.good() ){
        spdlog::error( "Unable to open {} for writing", _filePath.string() );
        return true;
    }
    file.seekp( ptr );

    if( sourceBuf == nullptr ){
        char zero = 0;
        for( uint32_t i = 0; i < spec.image_bytes(); ++i )
            file.write( &zero, sizeof( char ) );
        return true;
    }

    sourceBuf->read( 0, 0, true, spec.format );
    auto *tempBuf = new OIIO::ImageBuf;
    tempBuf->copy( *sourceBuf );
    channels( tempBuf, spec );
    scale( tempBuf, spec );

    // write the data to the smf
    file.write( (char *)tempBuf->localpixels(), spec.image_bytes() );
    file.close();

    tempBuf->clear();
    delete tempBuf;
    return false;
}

void
SMF::writeHeight( OIIO::ImageBuf *sourceBuf )
{
    SPDLOG_DEBUG( "Writing height" );
    _dirtyMask &= !SMF_HEIGHT;

    if( writeImage(_header.heightmapPtr, _heightSpec, sourceBuf ) ){
        spdlog::warn( "Wrote blank heightmap" );
    }
}

void
SMF::writeType( OIIO::ImageBuf *sourceBuf )
{
    SPDLOG_DEBUG( "INFO: Writing type" );
    _dirtyMask &= !SMF_TYPE;

    if( writeImage(_header.typeMapPtr, _typeSpec, sourceBuf ) ){
        spdlog::warn( "Wrote blank typemap" );
    }
}

//FIXME, the fact that this function can fail, but not notify it caller is annoying.
void
SMF::writeMini( OIIO::ImageBuf * sourceBuf )
{
    SPDLOG_DEBUG( "Writing mini" );
    _dirtyMask &= !SMF_MINI;

    std::fstream file( _filePath, std::ios::binary | std::ios::in | std::ios::out );
    if(! file.good() ){
        spdlog::error( "Unable to open {} for writing", _filePath.string() );
        return;
    }
    file.seekp( _header.minimapPtr );

    if( sourceBuf == nullptr ){
        char zero[ MINIMAP_SIZE ] = { 0 };
        file.write( zero, sizeof( zero ) );
        file.close();
        spdlog::warn( "Wrote blank minimap" );
        return;
    }

    sourceBuf->read( 0, 0, true, _miniSpec.format );
    auto *tempBuf = new OIIO::ImageBuf;
    tempBuf->copy( *sourceBuf );
    channels( tempBuf, _miniSpec );
    scale( tempBuf, _miniSpec );

    OIIO::ImageSpec spec;
    int blocks_size = 0;
    squish::u8 *blocks = nullptr;
    for( int i = 0; i < 9; ++i ){
        SPDLOG_DEBUG( "mipmap loop: {}", i );
        spec = tempBuf->specmod();

        blocks_size = squish::GetStorageRequirements(
                spec.width, spec.height, squish::kDxt1 );

        if( blocks == nullptr ){
            SPDLOG_DEBUG( "allocating space: {}", blocks_size );
            blocks = new squish::u8[ blocks_size ];
        }

        SPDLOG_DEBUG( "compressing to dxt1" );
        squish::CompressImage( (squish::u8 *)tempBuf->localpixels(),
                spec.width, spec.height, blocks, squish::kDxt1 );

        // Write data to smf
        SPDLOG_DEBUG( "writing dxt1 mip to file" );
        file.write( (char*)blocks, blocks_size );

        spec.width = spec.width >> 1;
        spec.height = spec.height >> 1;

        SPDLOG_DEBUG( "Scaling to: {}x{}", spec.width, spec.height );
        scale( tempBuf, spec );
    }
    delete blocks;
    delete tempBuf;

    file.close();
}

/// Write the tile header information to the smf
void
SMF::writeTileHeader()
{
    SPDLOG_DEBUG( "Writing tile reference information" );
    _dirtyMask &= !SMF_MAP_HEADER;

    std::fstream file( _filePath, std::ios::binary | std::ios::in | std::ios::out );
    file.seekp( _header.tilesPtr );

    // Tiles Header
    file.write( (char *)&_headerTiles, sizeof( SMF::HeaderTiles ) );

    // SMT Names & numbers
    for( auto [a,b] : _smtList ){
        file.write( (char *)&a, sizeof(a) );
        file.write( b.c_str(), b.size() + 1 );
    }
    file.close();
}

// write the tile-map information to the smf
void
SMF::writeMap( TileMap *tileMap )
{
    SPDLOG_DEBUG( "Writing map" );
    _dirtyMask &= !SMF_MAP;

    if( tileMap == nullptr ){
        writeImage( _mapPtr, _mapSpec, nullptr );
        spdlog::warn( "Wrote blank map" );
        return;
    }

    std::fstream file(_filePath, std::ios::binary | std::ios::in | std::ios::out);
    file.seekp( _mapPtr );
    file.write( (char *)tileMap->data(), _mapSpec.image_bytes() );
    file.close();

}

/// write the metal image to the smf
void
SMF::writeMetal( OIIO::ImageBuf *sourceBuf )
{
    SPDLOG_DEBUG( "Writing metal" );
    _dirtyMask &= !SMF_METAL;

    if( writeImage(_header.metalmapPtr, _metalSpec, sourceBuf ) )
        spdlog::warn( "Wrote blank metalmap" );
}

/// write the feature header information to the smf
// FIXME, this function can fail but it does not notify its caller
void
SMF::writeFeatures()
{
    SPDLOG_DEBUG( "Writing features" );
    _dirtyMask &= !SMF_FEATURES_HEADER;

    std::fstream file( _filePath, std::ios::binary | std::ios::in | std::ios::out );
    if(! file.good() ){
        spdlog::error( "Unable to open {} for writing", _filePath.string() );
        return;
    }
    file.seekp( _header.featurePtr );

    // set the current state
    _headerFeatures.nTypes = _featureTypes.size();
    _headerFeatures.nFeatures = _features.size();

    file.write( (char *)&_headerFeatures, sizeof( SMF::HeaderFeatures ) );
    for( const auto& i : _featureTypes ) file.write( i.c_str(), i.size() + 1 );
    for( auto i : _features ) file.write( (char *)&i, sizeof(SMF::Feature) );

    file.close();
}

// Write the grass image to the smf
void
SMF::writeGrass( OIIO::ImageBuf *sourceBuf ) {
    const auto &header = *std::find_if(extraHeaders.begin(), extraHeaders.end(),
        [](const auto &header){ return header->type == MEH_Vegetation; } );
    if( header ){
        auto headerGrass = static_cast<HeaderExtn_Grass *>( header.get() );
        SPDLOG_DEBUG( "Writing Grass" );
        if( writeImage( headerGrass->ptr, _grassSpec, sourceBuf ) ){
            spdlog::warn( "wrote blank grass map" );
        }
        _dirtyMask &= !SMF_GRASS;
    }
    return;
}

//FIXME returning nullptr on fail is crap.
OIIO::ImageBuf *
SMF::getImage( unsigned int ptr, const OIIO::ImageSpec& spec)
{
    std::ifstream file( _filePath );
    if(! file.good() ){
        spdlog::error( "Unable to open {} for reading", _filePath.string() );
        return nullptr;
    }

    auto *imageBuf = new OIIO::ImageBuf( spec );

    file.seekg( ptr );
    file.read( (char *)imageBuf->localpixels(), spec.image_bytes() );
    file.close();

    return imageBuf;
}

OIIO::ImageBuf *
SMF::getHeight( )
{
    return getImage(_header.heightmapPtr, _heightSpec );
}

OIIO::ImageBuf *
SMF::getType( )
{
    return getImage(_header.typeMapPtr, _typeSpec );
}

TileMap *
SMF::getMap( )
{
    auto *tileMap = new TileMap( _mapSpec.width, _mapSpec.height );
    std::fstream file( _filePath, std::ios::binary | std::ios::in );
    if(! file.good() ){
        spdlog::error( "Unable to open {} for reading", _filePath.string() );
        //FIXME returning nullptr is crap
        return nullptr;
    }
    file.seekg( _mapPtr );
    for( int y = 0; y < _mapSpec.height; ++y )
    for( int x = 0; x < _mapSpec.width; ++x ){
        file.read( (char *)&(*tileMap)( x, y ), 4 );
    }
    return tileMap;
}

OIIO::ImageBuf *SMF::getMini(){
    //TODO consider using openimageio dds reader to get minimap
    std::array< char, MINIMAP_SIZE > temp{};

    std::ifstream file( _filePath );
    if(! file.good() ){
        spdlog::error( "Unable to open {} for reading", _filePath.string() );
        //FIXME returning nullptr on return is crap. make return type a unique ptr and it will make more sense.
        return nullptr;
    }

    file.seekg( _header.minimapPtr );
    file.read( temp.data(), MINIMAP_SIZE );
    file.close();

    auto imageBuf = new OIIO::ImageBuf( _miniSpec );
    squish::DecompressImage( (squish::u8 *)imageBuf->localpixels(), 1024, 1024, temp.data(), squish::kDxt1);

    return imageBuf;
}

OIIO::ImageBuf *
SMF::getMetal( )
{
    return getImage(_header.metalmapPtr, _metalSpec );
}

std::string
SMF::getFeatureTypes( )
{
    std::stringstream list;
    for( const auto& i : _featureTypes ) list << i;
    return list.str();
}

std::string
SMF::getFeatures( )
{
    std::stringstream list;
    list << "NAME,X,Y,Z,ANGLE,SCALE\n";
    for( auto i : _features ){
        list << _featureTypes[ i.type ] << ","
             << i.x << ","
             << i.y << ","
             << i.z << ","
             << i.r << ","
             << i.s;
    }

    return list.str();
}

OIIO::ImageBuf *
SMF::getGrass() {
    const auto &header = *std::find_if(extraHeaders.begin(), extraHeaders.end(),
        [](const auto &header){ return header->type == MEH_Vegetation; } );
    if( header ){
        auto headerGrass = static_cast<HeaderExtn_Grass *>( header.get() );
        return getImage( headerGrass->ptr, _grassSpec );
    }
    return nullptr;
}
