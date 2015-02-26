#include <fstream>
#include <sstream>

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebufalgo.h>
#include <squish.h>

#include "elog/elog.h"

#include "config.h"
#include "smf.h"
#include "smt.h"
#include "util.h"

OIIO_NAMESPACE_USING
using namespace std;

void
SMF::good()
{
    if( dirtyMask & SMF_HEADER          ) LOG( INFO ) << "dirty header";
    if( dirtyMask & SMF_EXTRAHEADER     ) LOG( INFO ) << "dirty headerExtra";
    if( dirtyMask & SMF_HEIGHT          ) LOG( INFO ) << "dirty height";
    if( dirtyMask & SMF_TYPE            ) LOG( INFO ) << "dirty type";
    if( dirtyMask & SMF_MAP_HEADER      ) LOG( INFO ) << "dirty map header";
    if( dirtyMask & SMF_MAP             ) LOG( INFO ) << "dirty map";
    if( dirtyMask & SMF_MINI            ) LOG( INFO ) << "dirty mini";
    if( dirtyMask & SMF_METAL           ) LOG( INFO ) << "dirty metal";
    if( dirtyMask & SMF_FEATURES_HEADER ) LOG( INFO ) << "dirty features header";
    if( dirtyMask & SMF_FEATURES        ) LOG( INFO ) << "dirty features";
    if( dirtyMask & SMF_GRASS           ) LOG( INFO ) << "dirty grass";

}

SMF::~SMF()
{
    //delete extra headers
    for( auto i = headerExtras.begin(); i != headerExtras.end(); ++i ) {
        delete *i;
    }
}

bool
SMF::test( std::string fileName )
{
    char magic[ 16 ] = "";
    ifstream file( fileName );
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
SMF::create( string fileName, bool overwrite )
{
    SMF *smf;
    fstream file;

    // check for existing file and whether to overwrite
    file.open( fileName, ios::in );
    if( file.good() && !overwrite ) return NULL;
    file.close();

    DLOG( INFO ) << "Creating " << fileName;

    // attempt to create a new file or overwrite existing
    file.open( fileName, ios::binary | ios::out );
    if(! file.good() ){
        LOG( ERROR ) << "Unable to write to " << fileName;
        return NULL;
    }
    file.close();

    smf = new SMF;
    smf->fileName = fileName;
    smf->updateSpecs();
    smf->updatePtrs();
    smf->writeHeader();
    return smf;
}

SMF *
SMF::open( string fileName )
{
    SMF *smf;
    if( test( fileName ) ){
        DLOG( INFO ) << "Opening " << fileName;

        smf = new SMF;
        smf->dirtyMask = 0x00000000;
        smf->fileName = fileName;
        smf->read();
        return smf;
    }
    
    LOG( ERROR ) << "Cannot open " << fileName;
    return NULL;
}

void
SMF::read()
{
    FileMap map;
    uint32_t offset = 0;

    DLOG( INFO ) << "Reading " << fileName;
    ifstream file( fileName );
    CHECK( file.good() ) << "Unable to read" << fileName;
    
    file.seekg( 0, ios::end );
    // add block after the end of the file to test against.
    map.addBlock( file.tellg(), INT_MAX, "eof" );

    // read header structure.
    file.seekg(0);
    file.read( (char *)&header, sizeof(SMF::Header) );
    updateSpecs();

    // for each pointer, make sure they dont overlap with memory space of
    // other data
    map.addBlock(0,80, "header");
    map.addBlock( header.heightPtr, heightSpec.image_bytes(), "height" );
    map.addBlock( header.typePtr, typeSpec.image_bytes() , "type");
    map.addBlock( header.miniPtr, MINIMAP_SIZE, "mini" );
    map.addBlock( header.metalPtr, metalSpec.image_bytes(), "metal" );
    

    // Extra headers Information
    SMF::HeaderExtra *headerExtra;
    for(int i = 0; i < header.nHeaderExtras; ++i ) {
        headerExtra = new SMF::HeaderExtra;
        offset = file.tellg();
        file.read( (char *)headerExtra, sizeof(SMF::HeaderExtra) );
        file.seekg(offset);
        if(headerExtra->type == 1) {
            SMF::HeaderGrass *headerGrass = new SMF::HeaderGrass;
            file.read( (char *)headerGrass, sizeof(SMF::HeaderGrass));
            headerExtras.push_back( (SMF::HeaderExtra *)headerGrass );
            map.addBlock( headerGrass->ptr, grassSpec.image_bytes(), "grass" );
        }
        else {
            LOG( WARN ) << "Extra Header(" << i << ")"
                "has unknown type: " << headerExtra->type;
            headerExtras.push_back( headerExtra );
        }
        map.addBlock( offset, headerExtra->bytes, "extraheader" );
        file.seekg( offset + headerExtra->bytes );
        delete headerExtra;
    }

    // Tileindex Information
    file.seekg( header.tilesPtr );
    file.read( (char *)&headerTiles, sizeof( SMF::HeaderTiles ) );
    map.addBlock( header.tilesPtr, sizeof( SMF::HeaderTiles ), "mapHeader" );

    // TileFiles
    offset = file.tellg();
    uint32_t nTiles;
    char temp[1024];
    for( int i = 0; i < headerTiles.nFiles; ++i){
        file.read( (char *)&nTiles, 4 );
        this->nTiles.push_back( nTiles );
        file.getline( temp, 1023, '\0' );
        smtList.push_back( temp );
    }
    if( headerTiles.nFiles ){
        map.addBlock( offset, uint32_t( file.tellg() ) - offset, "tileFileList");
    }

    // while were at it lets get the file offset for the tilemap.
    mapPtr = file.tellg();
    map.addBlock( mapPtr, mapSpec.image_bytes(), "map" );

    // Featurelist information
    file.seekg( header.featuresPtr );
    file.read( (char *)&headerFeatures.nTypes, 4 );
    file.read( (char *)&headerFeatures.nFeatures, 4 );

    map.addBlock( header.featuresPtr, sizeof( SMF::HeaderFeatures ), "featuresHeader" );

    offset = file.tellg();
    for( int i = 0; i < headerFeatures.nTypes; ++i ){
        file.getline( temp, 255, '\0' );
        featureTypes.push_back( temp );
    }

    if( headerFeatures.nTypes)
        map.addBlock( offset, uint32_t( file.tellg() ) - offset, "featureNames" );

    SMF::Feature feature;
    for( int i = 0; i < headerFeatures.nFeatures; ++i ){
        file.read( (char *)&feature, sizeof(SMF::Feature) );
        features.push_back( feature );
    }

    if( headerFeatures.nFeatures )
        map.addBlock( file.tellg(), sizeof( SMF::Feature) * headerFeatures.nFeatures, "features" );

    file.close();
}

string
SMF::info()
{
    stringstream info;
    info << "[INFO]: " << fileName
         << "\n\tVersion: " << header.version
         << "\n\tID:      " << header.id

         << "\n\n\tWidth:          " << header.width
         << " | " << header.width / 64
         << "\n\tLength:         "   << header.length
         << " | " << header.length / 64
         << "\n\tSquareSize:     "   << header.squareWidth
         << "\n\tTexelPerSquare: "   << header.squareTexels
         << "\n\tTileSize:       "   << header.tileSize
         << "\n\tMinHeight:      "   << header.floor
         << "\n\tMaxHeight:      "   << header.ceiling

         << "\n\n\tHeightPtr:   "   << int_to_hex( header.heightPtr ) << " "
            << header.width+1 << "x"
            << header.length+1 << ":" << 1 << " UINT16"
         << "\n\tTypePtr:     "     << int_to_hex( header.typePtr ) << " " 
            << header.width << "x" << header.length << ":" << 1 << " UINT8"
         << "\n\tTilesPtr:    "     << int_to_hex( header.tilesPtr )
         << "\n\tMapPtr:      "     << int_to_hex( mapPtr ) << " "
            << header.width * 8 / header.tileSize << "x"
            << header.length * 8 / header.tileSize << ":" << 1 << " UINT32"
         << "\n\tMiniPtr:     "     << int_to_hex( header.miniPtr )
            << " " << 1024 << "x" << 1024 << ":" << 4 << " DXT1"
         << "\n\tMetalPtr:    "     << int_to_hex( header.metalPtr )
         << " " << header.width << "x" << header.length << ":" << 1 << "  UINT8"
         << "\n\tFeaturesPtr: "     << int_to_hex( header.featuresPtr )
         << "\n  HeaderExtras: "   << header.nHeaderExtras
        ;

    //HeaderExtras
    if( header.nHeaderExtras ){
        for( auto i = headerExtras.begin(); i != headerExtras.end(); ++i ){
            if( (*i)->type == 0 ){
                info << "\n    Null Header"
                     << "\n\tsize: " << (*i)->bytes
                     << "\n\ttype: " << (*i)->type
                    ;
            }
            else if( (*i)->type == 1 ){
                info << "\n    Grass"
                     << "\n\tsize: " << (*i)->bytes
                     << "\n\ttype: " << (*i)->type
                     << "\n\tptr:  " << int_to_hex( ((HeaderGrass *)(*i))->ptr )
                    ;
            }
            else {
                info << "\n    Unknown"
                     << "\n\tsize: " << (*i)->bytes
                     << "\n\ttype: " << (*i)->type
                    ;
            }
        }
    }

    // Tileindex Information
    info << "\n  Tile Index Information"
         << "\n\tTile Files:  " << headerTiles.nFiles
         << "\n\tTotal tiles: " << headerTiles.nTiles
        ;
    for( int i = 0; i < headerTiles.nFiles; ++i ){
        info << "\n\t    " << smtList[ i ] << ":" << nTiles[ i ] <<  endl;
    }

    // Features Information
    info << "\n  Features Information"
         << "\n\tFeatures: " << headerFeatures.nFeatures
         << "\n\tTypes:    " << headerFeatures.nTypes
        ;

    return info.str();
}

void
SMF::updateSpecs()
{
    DLOG( INFO ) << "Updating ImageSpec's";
    // Set heightSpec.
    heightSpec.width = header.width + 1;
    heightSpec.height = header.length + 1;
    heightSpec.nchannels = 1;
    heightSpec.set_format( TypeDesc::UINT16 );

    // set typeSpec
    typeSpec.width = header.width / 2;
    typeSpec.height = header.length / 2;
    typeSpec.nchannels = 1;
    typeSpec.set_format( TypeDesc::UINT8 );

    // set map spec
    mapSpec.width = header.width * 8 / header.tileSize;
    mapSpec.height = header.length * 8 / header.tileSize;
    mapSpec.nchannels = 1;
    mapSpec.set_format( TypeDesc::UINT );

    // set miniSpec
    miniSpec.width = 1024;
    miniSpec.height = 1024;
    miniSpec.nchannels = 4;
    miniSpec.set_format( TypeDesc::UINT8 );

    // set metalSpec
    metalSpec.width = header.width / 2;
    metalSpec.height = header.length / 2;
    metalSpec.nchannels = 1;
    metalSpec.set_format( TypeDesc::UINT8 );

    // set grassSpec
    grassSpec.width = header.width / 4;
    grassSpec.height = header.length / 4;
    grassSpec.nchannels = 1;
    grassSpec.set_format( TypeDesc::UINT8 );
}

void
SMF::updatePtrs()
{
    DLOG(INFO) << "Updating file offset pointers";

    header.heightPtr = sizeof( SMF::Header );
    
    for( auto i = headerExtras.begin(); i != headerExtras.end(); ++i )
        header.heightPtr += (*i)->bytes;

    header.typePtr = header.heightPtr + heightSpec.image_bytes();
    header.tilesPtr = header.typePtr + typeSpec.image_bytes();
    mapPtr = header.tilesPtr + sizeof( SMF::HeaderTiles );
    
    for( auto i = smtList.begin(); i != smtList.end(); ++i )
        mapPtr += i->size() + 5;
        
    header.miniPtr = mapPtr + mapSpec.image_bytes();
    header.metalPtr = header.miniPtr + MINIMAP_SIZE;
    header.featuresPtr = header.metalPtr + metalSpec.image_bytes();

    // features
    int eof;
    eof = header.featuresPtr + sizeof( SMF::HeaderFeatures );

    for( auto i = featureTypes.begin(); i != featureTypes.end(); ++i )
        eof += i->size() + 1;

    for( auto i = features.begin(); i != features.end(); ++i )
        eof += sizeof( SMF::Feature );

    // Optional Headers.
    for( auto i = headerExtras.begin(); i != headerExtras.end(); ++i ){
        if( (*i)->type == 1 ){
            HeaderGrass *headerGrass = reinterpret_cast<SMF::HeaderGrass *>(*i);
            headerGrass->ptr = eof;
            eof = headerGrass->ptr + grassSpec.image_bytes();
        }
    }
}

void
SMF::setFileName( std::string fileName )
{
    this->fileName = fileName;
    dirtyMask |= SMF_ALL;
}

void
SMF::setSize( int width, int length )
{
    if( header.width == width && header.length == length ) return;
    header.width = width * 64;
    header.length = length * 64;
    dirtyMask |= SMF_ALL;
}

void
SMF::setSquareWidth( int size )
{
    //FIXME
}

void
SMF::setSquareTexels( int size )
{
    //FIXME
}

void
SMF::setTileSize( int size )
{
    if( header.tileSize == size ) return;
    header.tileSize = size;
    dirtyMask |= SMF_HEADER;
    //FIXME this also effects the tilemap
}

void
SMF::setDepth( float floor, float ceiling )
{
    if( header.floor == floor && header.ceiling == ceiling ) return;
    header.floor = floor;
    header.ceiling = ceiling;
    dirtyMask |= SMF_HEADER;
}

void
SMF::enableGrass( bool enable )
{
    HeaderGrass *headerGrass = NULL;

    // get header if it exists. 
    for( auto i = headerExtras.begin(); i != headerExtras.end(); ++i ){
        if( (*i)->type == 1 ){
            headerGrass = (HeaderGrass *)*i;
            break;
        }
    }

    // if we have a header and we dont want it anymore
    if( !enable && headerGrass ){
        for( auto i = headerExtras.end(); i != headerExtras.begin(); --i ){
            if( (*i)->type == 1 ){
                headerGrass = (HeaderGrass *)*i;
                delete headerGrass;
                i = headerExtras.erase(i);
                --header.nHeaderExtras;
            }
        }
        dirtyMask |= SMF_ALL;
        return;
    }

    //if it doesnt exist, and we dont want it, do nothing
    if(! (enable || headerGrass) ) return;

    // otherwise we dont have and we want one
    headerGrass = new HeaderGrass();
    headerExtras.push_back( headerGrass );
    ++header.nHeaderExtras;
    dirtyMask |= SMF_ALL;
}

//TODO create a new function that Sets the map y depth and water level.

void
SMF::addTileFile( string fileName )
{
    dirtyMask |= SMF_MAP;

    if(! fileName.compare( "CLEAR" ) ){
        smtList.clear();
        nTiles.clear();
        headerTiles.nFiles = 0;
        headerTiles.nTiles = 0;
        return;
    }

    SMT *smt = NULL;
    CHECK( (smt = SMT::open( fileName )) ) << "Invalid smt file " << fileName;

    smtList.push_back( fileName );
    ++headerTiles.nFiles;

    nTiles.push_back( smt->nTiles);
    headerTiles.nTiles += smt->nTiles;

    delete smt;
}

void
SMF::addFeature( string name, float x, float y, float z, float r, float s )
{
    SMF::Feature feature;
    feature.x = x; feature.y = y; feature.z = z;
    feature.r = r; feature.z = s;

    bool match = false;
    for( unsigned int i = 0; i < featureTypes.size(); ++i ){
        if(! name.compare( featureTypes[ i ] ) ){
            match = true;
            feature.type = i;
            break;
        }
    }

    if(! match ){
        feature.type = featureTypes.size();
        featureTypes.push_back( name );
    }

    features.push_back( feature );

    headerFeatures.nTypes = featureTypes.size();
    headerFeatures.nFeatures = features.size();
    dirtyMask |= SMF_FEATURES;
}

void
SMF::addFeatures( string fileName )
{
    //Clear out the old list
    features.clear();
    featureTypes.clear();
    dirtyMask |= SMF_FEATURES;

    if(! fileName.compare("CLEAR") )return;

    // test the file
    fstream file( fileName, ifstream::in );
    CHECK( file.good() ) << "addFeatures: Cannot open " << fileName;

    // build inbuilt list
    char featureType[256];
    for( int i = 0; i < 16; ++i ){
        sprintf( featureType, "TreeType%i", i );
        featureTypes.push_back( featureType );
    }
    featureTypes.push_back("GeoVent");

    int n = 0;
    string cell;
    stringstream line;
    vector<string> tokens;
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
        catch ( std::invalid_argument ){
            LOG( WARN ) << "addFeatures: " << fileName << ", skipping invalid line at "
                << n;
            continue;
        }

    }
    file.close();

    DLOG( INFO )
        << "addFeatures"
        << "\n\tTypes: " << headerFeatures.nTypes
        << "\n\tTypes: " << headerFeatures.nFeatures;
}

void
SMF::writeHeader()
{
    DLOG( INFO ) << "Writing headers";

    header.id = rand();

    fstream file( fileName, ios::binary | ios::in | ios::out );
    CHECK( file.good() ) << "Unable to open " << fileName << " for writing";

    file.write( (char *)&header, sizeof(SMF::Header) );
    file.close();

    dirtyMask &= !SMF_HEADER;
}

void
SMF::writeExtraHeaders()
{
    DLOG( INFO ) << "Writing Extra Headers";

    fstream file( fileName, ios::binary | ios::in | ios::out );
    CHECK( file.good() ) << "Unable to open " << fileName << " for writing";

    file.seekp( sizeof( Header ) );
    for( auto eHeader = headerExtras.begin(); eHeader != headerExtras.end(); ++eHeader )
        file.write( (char *)*eHeader, (*eHeader)->bytes );

    file.close();

    dirtyMask &= !SMF_EXTRAHEADER;
}

bool
SMF::writeImage( unsigned int ptr, ImageSpec spec, ImageBuf *sourceBuf )
{
    fstream file( fileName, ios::binary | ios::in | ios::out );
    CHECK( file.good() ) << "Unable to open " << fileName << " for writing";
    file.seekp( ptr );

    if(! sourceBuf ){
        char zero = 0;
        for( uint32_t i = 0; i < spec.image_bytes(); ++i )
            file.write( &zero, sizeof( char ) );
        return true;
    }

    sourceBuf->read( 0, 0, true, spec.format );
    ImageBuf *tempBuf = new ImageBuf;
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
SMF::writeHeight( ImageBuf *sourceBuf )
{
    DLOG( INFO ) << "Writing height";
    dirtyMask &= !SMF_HEIGHT;

    if( writeImage( header.heightPtr, heightSpec, sourceBuf ) ){
        LOG( WARN ) << "Wrote blank heightmap";
    }
}

void
SMF::writeType( ImageBuf *sourceBuf )
{
    DLOG(INFO) << "INFO: Writing type";
    dirtyMask &= !SMF_TYPE;

    if( writeImage( header.typePtr, typeSpec, sourceBuf ) ){
        LOG( WARN ) << "Wrote blank typemap";    
    }
}

void
SMF::writeMini( ImageBuf * sourceBuf )
{
    DLOG( INFO ) << "Writing mini";
    dirtyMask &= !SMF_MINI;

    fstream file( fileName, ios::binary | ios::in | ios::out );
    CHECK( file.good() ) << "Unable to open " << fileName << " for writing";
    file.seekp( header.miniPtr );

    if(! sourceBuf ){
        char zero[ MINIMAP_SIZE ] = { 0 };
        file.write( zero, sizeof( zero ) );
        file.close();
        LOG( WARN ) << "Wrote blank minimap";
        return;
    }

    sourceBuf->read( 0, 0, true, miniSpec.format );
    ImageBuf *tempBuf = new ImageBuf;
    tempBuf->copy( *sourceBuf );
    channels( tempBuf, miniSpec );
    scale( tempBuf, miniSpec );

    ImageSpec spec;
    int blocks_size = 0;
    squish::u8 *blocks = NULL;
    for( int i = 0; i < 9; ++i ){
        DLOG( INFO ) << "mipmap loop: " << i;
        spec = tempBuf->specmod();

        blocks_size = squish::GetStorageRequirements(
                spec.width, spec.height, squish::kDxt1 );

        if(! blocks ){
            DLOG( INFO ) << "allocating space: " << blocks_size;
            blocks = new squish::u8[ blocks_size ];
        }

        DLOG( INFO ) << "compressing to dxt1";
        squish::CompressImage( (squish::u8 *)tempBuf->localpixels(),
                spec.width, spec.height, blocks, squish::kDxt1 );

        // Write data to smf
        DLOG( INFO ) << "writing dxt1 mip to file";
        file.write( (char*)blocks, blocks_size );

        spec.width = spec.width >> 1;
        spec.height = spec.height >> 1;

        DLOG( INFO ) << "Scaling to: " << spec.width << "x" << spec.height;
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
    DLOG( INFO ) << "Writing tile reference information";
    dirtyMask &= !SMF_MAP_HEADER;

    fstream file( fileName, ios::binary | ios::in | ios::out );
    file.seekp( header.tilesPtr );

    // Tiles Header
    file.write( (char *)&headerTiles, sizeof( SMF::HeaderTiles ) );

    // SMT Names & numbers
    for( int i = 0; i < headerTiles.nFiles; ++i ){
        int num = nTiles[i];
        file.write( (char *)&num, 4 );
        file.write( smtList[ i ].c_str(), smtList[ i ].size() + 1 );
    }
    file.close();
}

// write the tilemap information to the smf
void
SMF::writeMap( TileMap *tileMap )
{
    DLOG( INFO ) << "Writing map";
    dirtyMask &= !SMF_MAP;

    if(! tileMap ){
        writeImage( mapPtr, mapSpec, NULL);
        LOG( WARN ) << "Wrote blank map";
        return;
    }

    std::fstream file(fileName, std::ios::binary | std::ios::in | std::ios::out);
    file.seekp( mapPtr );
    file.write( (char *)tileMap->data(), mapSpec.image_bytes() );
    file.close();

}

/// write the metal image to the smf
void
SMF::writeMetal( ImageBuf *sourceBuf )
{
    DLOG( INFO ) << "Writing metal";
    dirtyMask &= !SMF_METAL;

    if( writeImage( header.metalPtr, metalSpec, sourceBuf ) )
        LOG( WARN ) << "Wrote blank metalmap";    
}

/// write the feature header information to the smf
void
SMF::writeFeaturesHeader()
{
    DLOG( INFO ) << "Writing feature headers";
    dirtyMask &= !SMF_FEATURES_HEADER;
        
    fstream file( fileName, ios::binary | ios::in | ios::out );
    CHECK( file.good() ) << "cannot open " << fileName << "for writing";
    file.seekp( header.featuresPtr );

    // Tiles Header
    headerFeatures.nTypes = featureTypes.size();
    headerFeatures.nFeatures = features.size();
    file.write( (char *)&headerFeatures, sizeof( SMF::HeaderFeatures ) );
    // SMT Names
    for( auto i = featureTypes.begin(); i != featureTypes.end(); ++i ){
        file.write( (*i).c_str(), (*i).size() + 1 );
    }
    file.close();
}

void
SMF::writeFeatures()
{
    DLOG( INFO ) << "Writing features";
    dirtyMask &= !SMF_FEATURES;

    fstream file( fileName, ios::binary | ios::in | ios::out );
    CHECK( file.good() ) << "cannot open " << fileName << "for writing";
    file.seekp( header.featuresPtr + 8 );

    for( auto i = featureTypes.begin(); i != featureTypes.end(); ++i ){
        file.write( i->c_str(), i->size() + 1 );
    }

    for( auto i = features.begin(); i != features.end(); ++i ){
        file.write( (char *)&(*i), sizeof(Feature) );
    }

    file.close();
}


// Write the grass image to the smf
void
SMF::writeGrass( ImageBuf *sourceBuf )
{
    HeaderGrass *headerGrass = NULL;

    // get header if it exists.
    for( auto i = headerExtras.begin(); i != headerExtras.end(); ++i ){
        if( (*i)->type == 1) headerGrass = (HeaderGrass *)*i;
    }

    //if it doesnt exist, do nothing.
    if(! headerGrass ) return;

    DLOG( INFO ) << "Writing Grass\n";

    if( writeImage( headerGrass->ptr, grassSpec, sourceBuf ) ){
        LOG( WARN ) << "wrote blank grass map";
    }

    dirtyMask &= !SMF_GRASS;
}


ImageBuf *
SMF::getImage( unsigned int ptr, ImageSpec spec)
{
    ifstream file( fileName );
    CHECK( file.good() ) << " Failed to open" << fileName << "for reading";

    ImageBuf *imageBuf = new ImageBuf( spec );

    file.seekg( ptr );
    file.read( (char *)imageBuf->localpixels(), spec.image_bytes() );
    file.close();

    return imageBuf;
}

ImageBuf *
SMF::getHeight( )
{
    return getImage( header.heightPtr, heightSpec );
}

ImageBuf *
SMF::getType( )
{ 
    return getImage( header.typePtr, typeSpec );
}

TileMap *
SMF::getMap( )
{
    TileMap *tileMap = new TileMap( mapSpec.width, mapSpec.height );
    std::fstream file( fileName, std::ios::binary | std::ios::in );
    CHECK( file.good() ) << " Failed to open" << fileName << "for reading";
    file.seekg( mapPtr );
    for( int y = 0; y < mapSpec.height; ++y )
    for( int x = 0; x < mapSpec.width; ++x ){
        file.read( (char *)&(*tileMap)( x, y ), 4 );
    }

    return tileMap;
}

ImageBuf *SMF::getMini(){
    ImageBuf * imageBuf = NULL;
    unsigned char data[1024 * 1024 * 4];

    ifstream file( fileName );
    CHECK( file.good() ) << " Failed to open" << fileName << "for reading";

    unsigned char *temp = new unsigned char[MINIMAP_SIZE];
    file.seekg( header.miniPtr );
    file.read( (char *)temp, MINIMAP_SIZE);

    squish::DecompressImage( (squish::u8 *)data, 1024, 1024, temp, squish::kDxt1);

    delete [] temp;
    // FIXME will this go out of context once the frame is complete?
    imageBuf = new ImageBuf( miniSpec, data );

    file.close();
    return imageBuf;
}

ImageBuf *
SMF::getMetal( )
{ 
    return getImage( header.metalPtr, metalSpec ); 
}

string
SMF::getFeatureTypes( )
{
    stringstream list;
    for( auto i = featureTypes.begin(); i != featureTypes.end(); ++i ){
        list << *i;
    }
    return list.str();
}

string
SMF::getFeatures( )
{
    stringstream list;
    list << "NAME,X,Y,Z,ANGLE,SCALE\n";
    for( auto i = features.begin(); i != features.end(); ++i ){
        list << featureTypes[i->type] << ","
             << i->x << ","
             << i->y << ","
             << i->z << ","
             << i->r << ","
             << i->s;
    }

    return list.str();
}

ImageBuf *
SMF::getGrass()
{
    HeaderGrass *headerGrass = NULL;
    for( auto i = headerExtras.begin();
            i != headerExtras.end(); ++i ){
        if( (*i)->type == 1 ){
            headerGrass = (HeaderGrass *)(*i);
            return getImage( headerGrass->ptr, grassSpec );
        }
    }
    return NULL;
}
