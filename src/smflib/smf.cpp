#include "smf.h"

#include "filemap.h"
#include "util.h"

namespace smflib {
void SMF::updateHeader() {
    SPDLOG_DEBUG( "Updating file offset pointers" );

    // The Height Map exists after the end of the header section, including if there are any extra headers
    _header.heightmapPtr = sizeof(_header);
    for( const auto& extraHeader : extraHeaders ) _header.heightmapPtr += extraHeader->size;

    // The Type map exists after the Height Map
    _header.typeMapPtr = _header.heightmapPtr + heightMapBytes();

    // We're going to skip the tileMap section for now and add that back at the end.
    // The MiniMap is next after the Type Map
    _header.minimapPtr = _header.typeMapPtr + typeMapBytes();

    // Afte the mini map, we'll do the metal map
    _header.metalmapPtr = _header.minimapPtr + miniMapBytes();

    // Mark the end of the file so we can use it later for the more dynamic sections.
    endStage1 = _header.metalmapPtr + metalMapBytes();

    // And if a grass Map exists we'll add that too.
    for( const auto& extraHeader : extraHeaders ) {
        if( extraHeader->type == MEH_Vegetation ) {
            const auto grassHeader = reinterpret_cast< Recoil::HeaderExtn_Grass* >(extraHeader.get());
            grassHeader->ptr       = _header.metalmapPtr + metalMapBytes();

            //Update the end of the file
            endStage1 = grassHeader->ptr + grassMapBytes();
        }
    }
}

void SMF::updatePtrs2() {
    // // We have added most of the fixed size portions, we'll add the tilemap, and the feature map now.
    // //FIXME _header.tilesPtr   = _header.typeMapPtr + static_cast< int >(_typeSpec.image_bytes());
    // _mapPtr = _header.tilesPtr + static_cast< int >(sizeof( Recoil::MapTileHeader ));
    //
    // for( const auto& name : _smtList | std::views::values ) {
    //     _mapPtr += static_cast< int >(sizeof( uint32_t ) + name.size() + 1);
    // }
    //
    // // eof is used here to help with optional extras like grass
    // // find out the expected end of the file
    // int eof = _header.featurePtr + static_cast< int >(sizeof( Recoil::MapFeatureHeader ));
    // for( const auto& i : _featureTypes ) eof += static_cast< int >(i.size()) + 1;
    // eof += static_cast< int >(_features.size()) * static_cast< int >(sizeof( Recoil::MapFeatureStruct ));
}

nlohmann::ordered_json SMF::json() {

    nlohmann::ordered_json j;
    j[ "filename" ] = _filePath;
    j[ "filesize" ] = std::format( "{}", humanise( file_size( _filePath ) ) );
    nlohmann::ordered_json h;
    h[ "version" ]         = _header.version;
    h[ "id" ]              = _header.mapid;
    h[ "width" ]           = std::format( "{} | {}", _header.mapx, _header.mapx / 64 );
    h[ "height" ]          = std::format( "{} | {}", _header.mapy, _header.mapy / 64 );
    h[ "squareSize" ]      = _header.squareSize;
    h[ "texelPerSquare" ]  = _header.texelPerSquare;
    h[ "tilesize" ]        = _header.tilesize;
    h[ "minHeight" ]       = _header.minHeight;
    h[ "maxHeight" ]       = _header.maxHeight;
    h[ "heightmapPtr" ]    = std::format( "{} | {}x{}:1 UINT16", to_hex( _header.heightmapPtr ), _header.mapx + 1, _header.mapy + 1 );
    h[ "typeMapPtr" ]      = std::format( "{} | {}x{}:1 UINT8", to_hex( _header.typeMapPtr ), _header.mapx, _header.mapy );
    h[ "tilesPtr" ]        = std::format( "{} | {}x{}:1 UINT32", to_hex( _header.tilesPtr ), _header.mapx * 8 / _header.tilesize, _header.mapy * 8 / _header.tilesize );
    h[ "minimapPtr" ]      = std::format( "{} | 1024x1024:4 DXT1", to_hex( _header.minimapPtr ) );
    h[ "metalmapPtr" ]     = std::format( "{} | {}x{}:1 UINT8", to_hex( _header.metalmapPtr ), _header.mapx, _header.mapy );
    h[ "featurePtr" ]      = std::format( "{}", to_hex( _header.featurePtr ) );
    h[ "numExtraHeaders" ] = _header.numExtraHeaders;
    j[ "header" ]          = h;

    //Header Extensions
    j[ "extraHeaders" ] = nlohmann::json::array();
    for( const auto& extraHeader : extraHeaders ) {
        nlohmann::ordered_json eh;
        switch( extraHeader->type ) {
        case MEH_None: {
            eh[ "size" ] = extraHeader->size;
            eh[ "type" ] = fmt::format( "{} - Null Header", extraHeader->type );
        }
        break;
        case MEH_Vegetation: {
            const auto header = reinterpret_cast< Recoil::HeaderExtn_Grass* >(extraHeader.get());
            eh[ "size" ]      = extraHeader->size;
            eh[ "type" ]      = fmt::format( "{} - MEH_Vegetation", extraHeader->type );
            eh[ "ptr" ]       = to_hex( header->ptr );
        }
        break;
        default: {
            eh[ "size" ] = extraHeader->size;
            eh[ "type" ] = fmt::format( "{} - Unknown", extraHeader->type );
        }
        }
        j[ "extraHeaders" ] += eh;
    }

    // Component Info
    if( _heightIO ) j[ "HeightMap Info" ]  = _heightIO->json();
    if( _typeIO ) j[ "TypeMap Info" ]  = _typeIO->json();
    if( _miniIO ) j[ "MiniMap Info" ]  = _miniIO->json();
    if( _metalIO ) j[ "MetalMap Info" ]  = _metalIO->json();
    if( _grassIO ) j[ "GrassMap Info" ]  = _grassIO->json();
    if( _tileIO ) j[ "TileMap Info" ]  = _tileIO->json();
    if( _featureIO ) j[ "Feature Info" ]  = _featureIO->json();
    return j;
}

void SMF::setMapSize( int width, int height ) {}
void SMF::setHeight( float lower, float upper ) {}


const std::filesystem::path& SMF::get_file_path() const { return _filePath; }

/**
 * \brief
 * \param file_path
 */
void SMF::set_file_path( const std::filesystem::path& file_path ) {
    _filePath = file_path;
}

void SMF::setHeightIO( const std::shared_ptr< SMFIOBase >& height_io ) {
    _heightIO = height_io;
    _heightIO->setSMF( this );
}
void SMF::setTypeIO( const std::shared_ptr< SMFIOBase >& type_io ) {
    _typeIO = type_io;
    _typeIO->setSMF( this );
}
void SMF::setMiniIO( const std::shared_ptr< SMFIOBase >& mini_io ) {
    _miniIO = mini_io;
    _miniIO->setSMF( this );
}
void SMF::setMetalIO( const std::shared_ptr< SMFIOBase >& metal_io ) {
    _metalIO = metal_io;
    _metalIO->setSMF( this );
}
void SMF::setGrassIO( const std::shared_ptr< SMFIOBase >& grass_io ) {
    _grassIO = grass_io;
    _grassIO->setSMF( this );
}
void SMF::setTileIO( const std::shared_ptr< SMFIOBase >& tile_io ) {
    _tileIO = tile_io;
    _tileIO->setSMF( this );
}
void SMF::setFeatureIO( const std::shared_ptr< SMFIOBase >& feature_io ) {
    _featureIO = feature_io;
    _featureIO->setSMF( this );
}

/**
 * \brief
 * \param components
 */
void SMF::read( const int components ) {
    SPDLOG_DEBUG( "Reading file: {}", _filePath.string() );
    std::ifstream file( _filePath );
    if( !file.good() ) {
        SPDLOG_ERROR( "unable to read: {}", _filePath.string() );
    }

    // add block after the end of the file to test against.
    file.seekg( 0, std::ios::end );
    _fileSize = file.tellg();
    _fileMap->addBlock( _fileSize, INT_MAX, "eof" );

    if( components & HEADER ) {
        SPDLOG_INFO( "Reading header information" );
        // read header structure.
        file.seekg( 0 );
        file.read( reinterpret_cast< char* >(&_header), sizeof(_header) );

        _fileMap->addBlock( 0, 80, "header" );

        if( _heightIO ) _heightIO->update();
        if( _typeIO ) _typeIO->update();
        if( _miniIO ) _miniIO->update();
        if( _metalIO ) _metalIO->update();

        // for each pointer, make sure they don't overlap with memory space of
        // other data
        _fileMap->addBlock( _header.heightmapPtr, heightMapBytes() , "height" );
        _fileMap->addBlock( _header.typeMapPtr, typeMapBytes(), "type" );
        _fileMap->addBlock( _header.minimapPtr, miniMapBytes(), "mini" );
        _fileMap->addBlock( _header.metalmapPtr, metalMapBytes(), "metal" );
    }

    if( components & EXTRA_HEADER && _header.numExtraHeaders > 0 ) {
        file.seekg( sizeof(Recoil::SMFHeader) );
        // Extra headers Information
        for( int i = 0; i < _header.numExtraHeaders; ++i ) {
            const uint32_t headerStart = file.tellg();
            auto headerStub       = std::make_unique< Recoil::ExtraHeader >( 0, 0 );
            file.read( reinterpret_cast< char* >(headerStub.get()), sizeof(*headerStub) );

            switch( headerStub->type ) {
            case MEH_Vegetation: {
                SPDLOG_INFO( "Found grass header" );
                auto header = std::make_unique< Recoil::HeaderExtn_Grass >();
                // re-read the header into the appropriate container
                file.seekg( headerStart );
                file.read( reinterpret_cast< char* >(header.get()), sizeof(*header) );
                if( header->size != 12 )SPDLOG_WARN( "Grass Header has incorrect size={}, should be {}", header->size, sizeof(Recoil::HeaderExtn_Grass) );
                _fileMap->addBlock( headerStart, header->size, "MEH_Vegetation" );
                _fileMap->addBlock( header->ptr, grassMapBytes(), "GrassMapData" );
                extraHeaders.push_back( std::move( header ) );
                if( _grassIO )_grassIO->update();
                break;
            }
            default: {
                SPDLOG_WARN( "Extra Header({}) has unknown type: {}", i, headerStub->type );
                //move the read head to the end of the header
                file.seekg( headerStart + headerStub->size );
                _fileMap->addBlock( headerStart, headerStub->size, "MEH_UNKNOWN" );
                extraHeaders.push_back( std::move( headerStub ) ); //save the stub for later
            }
            }
        }
    }
    if( components & HEIGHT ) {
        _heightIO->read( file );
    }
    // Type
    if( components & TYPE ) {
        _typeIO->read( file );
    }
    // Mini
    if( components & MINI ) {
        _miniIO->read( file );
    }
    // Metal
    if( components & METAL ) {
        _metalIO->read( file );
    }
    // grass
    if( components & GRASS ) {
        _grassIO->read( file );
    }
    // Tile
    if( components & TILE ) {
        _tileIO->read( file );
    }
    // Features
    if( components & FEATURES ) {
        _featureIO->read( file );
    }

    file.close();
}

void SMF::write( std::bitset< 12 > components ) {}
}
