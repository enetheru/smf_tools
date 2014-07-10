#include "config.h"
#include "smf.h"
#include "smt.h"

#include <fstream>
#include <sstream>

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebufalgo.h>
OIIO_NAMESPACE_USING

#include <nvtt/nvtt.h>
#include "nvtt_output_handler.h"
#include "dxt1load.h"
#include "util.h"

/// Sets the dirty flag so that we know where to re-write
/* The dirst flag will always be the lower of it current state
 * or the requested state
 */
void SMF::setDirty( int i ){
    dirty = fmin( dirty, i );
}

/// get the dirty state
int SMF::getDirty(){
    return dirty;
}

/// Creates an SMF file and returns a pointer to management class
/*
 */
SMF *SMF::create( string fileName, bool overwrite, bool verbose, bool quiet ){
    SMF *smf;
    fstream file;
    
    // check for existing file and whether to overwrite
    file.open( fileName, ios::in );
    if( file.good() && !overwrite ) return NULL;
    file.close();

    if( verbose ) cout << "INFO: Creating " << fileName << endl;

    // attempt to create a new file or overwrite existing
    file.open( fileName, ios::binary | ios::out );
    if(! file.good() ){
        if(! quiet ) cout << "ERROR: Unable to write to " << fileName << endl;
        return NULL;
    }
    file.close();

    smf = new SMF( fileName, verbose, quiet );
    smf->updatePtrs();
    smf->writeHeaders();
    smf->writeHeight(NULL);
    smf->writeType(NULL);
    smf->writeTileHeader();
    smf->writeMap(NULL);
    smf->writeMini(NULL);
    smf->writeMetal(NULL);
    smf->writeFeaturesHeader();
    smf->writeFeatures();
    smf->init = true;
    return smf;
}

/// Opens an existing SMF file and returns pointer to management class
/*
 */
SMF *SMF::open( string fileName, bool verbose, bool quiet ){
    bool good = false;
    SMF *smf;

    char magic[ 16 ] = "";
    ifstream file( fileName );
    if( file.good() ){
        file.read( magic, 16 );
        if(! strcmp( magic, "spring map file" ) ){
            good = true;
            file.close();
        }
    }

    if( good ){
        if( verbose ) cout << "INFO: Opening " << fileName << endl;
        smf = new SMF(fileName, verbose, quiet );
        smf->init = !smf->read();
        return smf;
    }
    else {
        if( verbose ) cout << "INFO: Cannot open " << fileName << endl;
    }
    return NULL;
}

/// Reads in the data from the file on disk
/*
 */
bool SMF::read(){
    int offset;

    if( verbose ) cout << "INFO: Reading " << fileName << endl;
    ifstream file( fileName );
    file.seekg(0);
    file.read( (char *)&header, sizeof(SMF::Header) );

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
        }
        else {
            if( verbose ) cout << "WARNING: unknown header type " << i << endl;
            headerExtras.push_back( headerExtra );
        }
        file.seekg( offset + headerExtra->size);
        delete headerExtra;
    }

    updateSpecs();

    // Tileindex Information
    file.seekg( header.tilesPtr );
    file.read( (char *)&headerTiles, sizeof( SMF::HeaderTiles ) );
    
    // TileFiles
    int nTiles;
    char temp[256];
    for( int i = 0; i < headerTiles.nFiles; ++i){
        file.read( (char *)&nTiles, 4 );
        this->nTiles.push_back( nTiles );
        file.getline( temp, 255, '\0' );
        smtList.push_back( temp );
    }

    // whle were at it lets get the file offset for the tilemap.
    mapPtr = file.tellg();

    // Featurelist information
    file.seekg( header.featuresPtr );
    file.read( (char *)&headerFeatures.nTypes, 4 );
    file.read( (char *)&headerFeatures.nFeatures, 4 );

    offset = file.tellg();
    file.seekg( 0, ios::end );
    int filesize = file.tellg();
    file.seekg( offset );

    int eeof = offset + headerFeatures.nFeatures * sizeof(SMF::Feature);
    if( eeof > filesize ) {
        if(! quiet ){
            cout << "WARNING: Filesize is not large enough to contain the reported number of features. Ignoring feature data.\n";
        }
    }
    else {
        for( int i = 0; i < headerFeatures.nTypes; ++i ){
            file.getline( temp, 255, '\0' );
            featureTypes.push_back( temp );
        }

        SMF::Feature feature;
        for( int i = 0; i < headerFeatures.nFeatures; ++i ){
            file.read( (char *)&feature, sizeof(SMF::Feature) );
            features.push_back( feature );
        }
    }

    file.close();
    return false;
}

/// Output map information as string
/**
 */
string SMF::info(){
    stringstream info;
    info << "INFO: " << fileName 
         << "\n\tVersion: " << header.version
         << "\n\tID:      " << header.id

         << "\n\n\tWidth:          " << header.width
         << "\n\tLength:         "   << header.length 
         << "\n\tSquareSize:     "   << header.squareWidth 
         << "\n\tTexelPerSquare: "   << header.squareTexels 
         << "\n\tTileSize:       "   << header.tileRes 
         << "\n\tMinHeight:      "   << header.floor 
         << "\n\tMaxHeight:      "   << header.ceiling 

         << "\n\n\tHeightPtr:    "   << int_to_hex( header.heightPtr )
         << "\n\tTypePtr:      "     << int_to_hex( header.typePtr )
         << "\n\tTilesPtr:     "     << int_to_hex( header.tilesPtr )
         << "\n\tMapPtr:       "     << int_to_hex( mapPtr )
         << "\n\tMiniPtr:      "     << int_to_hex( header.miniPtr )
         << "\n\tMetalPtr:     "     << int_to_hex( header.metalPtr )
         << "\n\tFeaturesPtr:  "     << int_to_hex( header.featuresPtr )
         << endl;

    //HeaderExtras
    if( header.nHeaderExtras ){
        for( auto i = headerExtras.begin(); i != headerExtras.end(); ++i ){
            if( (*i)->type == 0 ){
                info << "    Null Header"
                     << "\n\tsize: " << (*i)->size
                     << "\n\ttype: " << (*i)->type
                     << endl;
            }
            else if( (*i)->type == 1 ){
                info << "    Grass"
                     << "\n\tsize: " << (*i)->size
                     << "\n\ttype: " << (*i)->type
                     << "\n\tptr:  " << int_to_hex( ((HeaderGrass *)(*i))->ptr )
                     << endl;
            }
            else {
                info << "    Unknown"
                     << "\n\tsize: " << (*i)->size
                     << "\n\ttype: " << (*i)->type
                     << endl;
            }
 
        }
    }

    // Tileindex Information
    info << "\n  HeaderExtras: "   << header.nHeaderExtras
         << "\n    Tile Index Information"
         << "\n\tTile Files:  " << headerTiles.nFiles
         << "\n\tTotal tiles: " << headerTiles.nTiles
         << endl;
    for( int i = 0; i < headerTiles.nFiles; ++i ){
        info << "\t" << smtList[ i ] << ":" << nTiles[ i ] <<  endl;
    }  

    // Features Information
    info << "\n    Features Information"
         << "\n\tFeatures: " << headerFeatures.nFeatures
         << "\n\tTypes:    " << headerFeatures.nTypes
         << endl;

    // TODO put into stream the information for the rest of the file structure.
    return info.str();
}

/// Re-writes the data to the disk taking into consideration existing data and preserving file order
/*
 */
bool SMF::reWrite( ){
    ImageBuf *height, *type, *map, *mini, *metal, *grass = NULL;
    // 0: from the very beginning full re-write
    // 1: extra headers onwards
    // 2: tile headers onwards
    // 3: feature headers onwards
    
    // First switch gathers the data from the file that may be overwritten
    // Second switch writes to the file from the gathered data

    switch( getDirty() ){
        case INT_MAX:
            return false;
        case 0:
        case 1: 
            height = getHeight();
            type = getType();
        case 2: 
            map = getMap();
            mini = getMini();
            metal = getMetal();
        case 3: 
            grass = getGrass();
    }

    updatePtrs();
    if( verbose ) cout << "INFO: (Re-)Writing " << fileName << endl;

    writeHeaders();
    switch( getDirty() ){
        case 0:
        case 1:
            writeHeight( height );
            writeType( type );
        case 2:
            writeTileHeader();
            writeMap( map );
            writeMini( mini );
            writeMetal( metal );
        case 3:
            writeFeaturesHeader();
            writeFeatures();
            if( grass ) writeGrass( grass );
    }
    dirty = INT_MAX;
    return false;
}

/// Update the data block sizes
/** Its convenient to know this information off hand, rather than recalculate
 * in every function we need it in.
 */
void SMF::updateSpecs(){
    if( verbose ) cout << "INFO: Updating Specifications" << endl;
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

    // set miniSpec
    miniSpec.width = 1024;
    miniSpec.height = 1024;
    miniSpec.nchannels = 4;
    miniSpec.set_format( TypeDesc::UINT8 );

    // set mapSpec
    mapSpec.width = header.width * 4 / header.tileRes;
    mapSpec.height = header.length * 4 / header.tileRes;
    mapSpec.nchannels = 1;
    mapSpec.set_format( TypeDesc::UINT );

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

/// Update the file offset pointers
/** This function makes sure that all data offset pointers are pointing to the
 * correct location and should be called whenever changes to the class are
 * made that will effect its values.
 */
void SMF::updatePtrs(){
    updateSpecs();
    if( verbose ) cout << "INFO: Updating file offsets" << endl;

    header.heightPtr = sizeof( SMF::Header );
    for( auto i = headerExtras.begin(); i != headerExtras.end(); ++i )
        header.heightPtr += (*i)->size;

    header.typePtr = header.heightPtr + heightSpec.image_bytes();

    header.tilesPtr = header.typePtr + typeSpec.image_bytes(); 

    mapPtr = header.tilesPtr + sizeof( SMF::HeaderTiles );

    for( auto i = smtList.begin(); i != smtList.end(); ++i )
        mapPtr += (*i).size() + 5;

    header.miniPtr = mapPtr + mapSpec.image_bytes();

    header.metalPtr = header.miniPtr + MINIMAP_SIZE;

    header.featuresPtr = header.metalPtr + metalSpec.image_bytes();

    int eof;
    eof = header.featuresPtr + sizeof( SMF::HeaderFeatures );
    for( auto i = featureTypes.begin(); i != featureTypes.end(); ++i )
        eof += (*i).size() + 1;
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

/// Set the map x and z depth
void SMF::setSize( int width, int length ){
    header.width = width * 64;
    header.length = length * 64;
    setDirty( 0 );
    reWrite();
}

/// Set the map y depth
void SMF::setDepth( float floor, float ceiling ){
    header.floor = floor;
    header.ceiling = ceiling;
    writeHeaders();
}

///TODO Set the map y depth and water level.

/// Add Tile files
bool SMF::addTileFile( string fileName ){
    SMT *smt = NULL;

    if(! fileName.compare( "CLEAR" ) ){
        smtList.clear();
        nTiles.clear();
        headerTiles.nFiles = 0;
        headerTiles.nTiles = 0;
        setDirty( 2 );
        return false;
    }

    if(! (smt = SMT::open( fileName, verbose, quiet )) ){
        if(! quiet ) cout << "ERROR: invalid smt file " << fileName << endl;
        return true;
    }

    smtList.push_back( fileName );
    ++headerTiles.nFiles;

    nTiles.push_back( smt->getNTiles() );
    headerTiles.nTiles += smt->getNTiles();

    delete smt;
    setDirty( 2 );
    return false;
}

void SMF::addFeature( string name, float x, float y, float z, float r, float s ){
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
    setDirty( 3 );
}

bool SMF::writeHeaders(){
    if( verbose ) cout << "INFO: Writing headers\n";

    header.id = rand();

    fstream file( fileName, ios::binary | ios::in | ios::out );
    if(! file.good() ){
        if(! quiet ) cout << "ERROR: unable to open file for writing\n";
        return true;
    }
    //file.seekp(0);
    file.write( (char *)&header, sizeof(SMF::Header) );

    for( auto eHeader = headerExtras.begin(); eHeader != headerExtras.end(); ++eHeader )
        file.write( (char *)*eHeader, (*eHeader)->size );

    file.close();
    return false;
}

bool SMF::writeImage( unsigned int ptr, ImageSpec spec, ImageBuf *sourceBuf ){
    if( sourceBuf ) sourceBuf->read( 0, 0, true, spec.format );
    ImageBuf *imageBufa = channels( sourceBuf, spec );
    ImageBuf *imageBufb = scale( imageBufa, spec );
    delete imageBufa;


    // write the data to the smf
    fstream file( fileName, ios::binary | ios::in | ios::out );
    file.seekp( ptr );
    file.write( (char *)imageBufb->localpixels(), spec.image_bytes() );
    file.close();

    delete imageBufb;
    return false;
}

bool SMF::writeHeight( ImageBuf *sourceBuf ){
    if( verbose ) cout << "INFO: Writing height\n";
    return writeImage( header.heightPtr, heightSpec, sourceBuf );   
}

bool SMF::writeType( ImageBuf *sourceBuf ){
    if( verbose ) cout << "INFO: Writing type\n";
    return writeImage( header.typePtr, typeSpec, sourceBuf );   
}

/// Writes an image to the minimap data point in the smf
/**
 */
bool SMF::writeMini( ImageBuf * sourceBuf ){
    if( verbose )cout << "INFO: Writing mini\n";

    //TODO attempt to generate minimap from whatever is available and overlay the
    // map title
    ImageBuf *tempBufa = channels( sourceBuf, miniSpec );    
    ImageBuf *miniBuf = scale( tempBufa, miniSpec );  
    delete tempBufa;  

    // setup DXT1 Compression
    nvtt::InputOptions inputOptions;
    inputOptions.setTextureLayout( nvtt::TextureType_2D, miniSpec.width, miniSpec.height );
    inputOptions.setMipmapData( miniBuf->localpixels(), miniSpec.width, miniSpec.height );

    nvtt::CompressionOptions compressionOptions;
    compressionOptions.setFormat( nvtt::Format_DXT1 );
    if( slowcomp ) compressionOptions.setQuality( nvtt::Quality_Normal ); 
    else compressionOptions.setQuality( nvtt::Quality_Fastest ); 

    nvtt::OutputOptions outputOptions;
    outputOptions.setOutputHeader( false );

    NVTTOutputHandler *outputHandler = new NVTTOutputHandler( MINIMAP_SIZE + 1);
    outputOptions.setOutputHandler( outputHandler );

    nvtt::Compressor compressor;
    compressor.process( inputOptions, compressionOptions, outputOptions );

    // Write data to smf
    fstream file( fileName, ios::binary | ios::in | ios::out );
    file.seekp( header.miniPtr );
    file.write( outputHandler->getBuffer(), MINIMAP_SIZE );
    file.close();

    delete outputHandler;
    delete miniBuf;

    return false;
}

/// Write the tile header information to the smf
bool SMF::writeTileHeader(){
    if( verbose ) cout << "INFO: Writing tile reference information\n";
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

    return false;
}

// write the tilemap information to the smf
bool SMF::writeMap( ImageBuf *sourceBuf ){
    if( verbose ) cout << "INFO: Writing map\n";
    return writeImage( mapPtr, mapSpec, sourceBuf );   
}

/// write the metal image to the smf
bool SMF::writeMetal( ImageBuf *sourceBuf ){
    if( verbose ) cout << "INFO: Writing metal\n";
    return writeImage( header.metalPtr, metalSpec, sourceBuf );   
}

/// write the feature header information to the smf
bool SMF::writeFeaturesHeader() {
    if( verbose ) cout << "INFO: Writing feature headers\n";
    fstream file( fileName, ios::binary | ios::in | ios::out );
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

    return false;
}

// write the featurelist to the smf
bool SMF::writeFeatures() {
    if( verbose ) cout << "INFO: Writing features\n";
    //TODO fix stub
    return false;
}

/// Write the grass image to the smf
/*
 */
bool SMF::writeGrass( ImageBuf *sourceBuf ) {
    HeaderGrass *headerGrass = NULL;
    bool rewrite = false;

    // get header if it exists.
    for( auto i = headerExtras.begin(); i != headerExtras.end(); ++i ){
        if( (*i)->type == 1) headerGrass = (HeaderGrass *)*i;
    }

    if(! sourceBuf && headerGrass ){
        for( auto i = headerExtras.begin(); i != headerExtras.end(); ++i ){
            if( (*i)->type == 1 ){
                i = headerExtras.erase(i);
                --i;
                --header.nHeaderExtras;
            }
        }
        setDirty( 0 );
        reWrite();
        return false;
    }
    else if(! sourceBuf && ! headerGrass ) return false;

    if( verbose ) cout << "INFO: Writing Grass\n";

    // else create one.
    if(! headerGrass ){
        rewrite = true;
        headerGrass = new HeaderGrass();
        headerExtras.push_back( headerGrass );
        ++header.nHeaderExtras;
    }

    if(! rewrite )
        return writeImage( headerGrass->ptr, grassSpec, sourceBuf );
    else
        setDirty( 0 );
        reWrite();   

    return false;
}

/* bool SMF::saveFeatures(){
    if( verbose )cout << "INFO: saveFeatures\n";

    char filename[256];
    sprintf( filename, "%s.smf", outPrefix.c_str() );

    fstream smf(filename, ios::binary | ios::in | ios::out);
    smf.seekp(featuresPtr);

    int nTypes = featureTypes.size();
    smf.write( (char *)&nTypes, 4); //featuretypes
    int nFeatures = features.size();
    smf.write( (char *)&nFeatures, sizeof(nFeatures)); //numfeatures
    if( verbose ) printf( "    %i features, %i types.\n", nFeatures, nTypes);

    for(unsigned int i = 0; i < featureTypes.size(); ++i ) {
        smf.write(featureTypes[i].c_str(), featureTypes[i].size() + 1 );
    }

    for(unsigned int i = 0; i < features.size(); ++i ) {
        smf.write( (char *)&features[i], sizeof(SMFFeature) );
    }

    smf.write( "\0", sizeof("\0"));
    smf.close();
    return false;
}
*/

/* bool SMF::decompileFeaturelist(int format = 0) {
    if( features.size() == 0) return true;
    if( verbose )cout << "INFO: Decompiling Feature List. "
        << featuresPtr << endl;

    char filename[256];
    if(format == 0)
        sprintf( filename, "%s_featurelist.csv", outPrefix.c_str() );
    else if (format == 1)
        sprintf( filename, "%s_featurelist.lua", outPrefix.c_str() );

    ofstream outfile(filename);

    ifstream smf( fileName, ifstream::in );
    if( !smf.good() ) {
        return true;
    }

    smf.seekg( featuresPtr );

    int nTypes;
    smf.read( (char *)&nTypes, 4);

    int nFeatures;
    smf.read( (char *)&nFeatures, 4);

    char name[256];
    vector<string> featureNames;
    for( int i = 0; i < nTypes; ++i ) {
        smf.getline(name,255, '\0');
        featureNames.push_back(name);
    }

    char line[1024];
    SMFFeature feature;
    for( int i=0; i < nFeatures; ++i ) {
//        READ_SMFFEATURE(temp,&smf);
        smf.read( (char *)&feature, sizeof(SMFFeature) );

        if(format == 0) {
                outfile << featureNames[feature.type] << ',';
                outfile << feature.x << ',';
                outfile << feature.y << ',';
                outfile << feature.z << ',';
                outfile << feature.r << ',';
                outfile << feature.s << endl;
            } else if (format == 1) {
                sprintf(line, "\t\t{ name = '%s', x = %i, z = %i, rot = \"%i\",},\n",
                    featureNames[feature.type].c_str(),
                    (int)feature.x, (int)feature.z, (int)feature.r * 32768 );
                outfile << line;
            }

    }
    smf.close();
    outfile.close();

    return false;
}
*/

ImageBuf *SMF::getImage( unsigned int ptr, ImageSpec spec){
    ifstream file( fileName );
    if(! file.good() ) return NULL;

    ImageBuf *imageBuf = new ImageBuf( spec );

    file.seekg( ptr );
    file.read( (char *)imageBuf->localpixels(), spec.image_bytes() );
    file.close();

    return imageBuf;
}

ImageBuf *SMF::getHeight( ){ return getImage( header.heightPtr, heightSpec ); }
ImageBuf *SMF::getType( ){ return getImage( header.typePtr, typeSpec ); }

ImageBuf *SMF::getMini(){
    ImageBuf * imageBuf = NULL;
    unsigned char *data;

    ifstream smf( fileName );
    if( smf.good() ) {
        unsigned char *temp = new unsigned char[MINIMAP_SIZE];
        smf.seekg( header.miniPtr );
        smf.read( (char *)temp, MINIMAP_SIZE);
        data = dxt1_load(temp, 1024, 1024);
        delete [] temp;
        imageBuf = new ImageBuf( miniSpec, data );
    }
    smf.close();
    return imageBuf;
}

ImageBuf *SMF::getMap( ){ return getImage( mapPtr, mapSpec ); }
ImageBuf *SMF::getMetal( ){ return getImage( header.metalPtr, metalSpec ); }

ImageBuf *SMF::getGrass(){
    bool found = false;

    HeaderGrass *headerGrass;
    for( auto i = headerExtras.begin();
            i != headerExtras.end(); ++i ){
        if( (*i)->type == 1 ){
            headerGrass = (HeaderGrass *)(*i);
            found = true;
            break;
        }
    }
    if(! found ) return NULL;

    return (getImage( headerGrass->ptr, grassSpec ));
}

//FIXME incorporate
/*    // Process Features //
    //////////////////////
    if( featurelist ) {
        if( verbose ) cout << "INFO: Processing and writing features." << endl;

        MapFeatureHeader mapFeatureHeader;
        vector<MapFeatureStruct> features;
        MapFeatureStruct *feature;
        vector<string> featurenames;

        string line;
        string cell;
        vector<string> tokens;
        char value[256];

        // build inbuilt list
        for ( int i=0; i < 16; i++ ) {
            sprintf(value, "TreeType%i", i);
            featurenames.push_back(value);
        }
        featurenames.push_back("GeoVent");

        // Parse the file
        printf( "INFO: Reading %s\n", featurelist_fn.c_str() );
        ifstream flist(featurelist_fn.c_str(), ifstream::in);

        while ( getline( flist, line ) ) {
            stringstream lineStream( line );
            feature = new MapFeatureStruct;
            mapFeatureHeader.numFeatures++;

            tokens.clear();
            while( getline( lineStream, cell, ',' ) ) tokens.push_back( cell );

            if(!featurenames.empty()) {
                for (unsigned int i=0; i < featurenames.size(); i++)
                    if( !featurenames[i].compare( tokens[0] ) ) {
                        feature->featureType = i;
                        break;
                    } else {
                        featurenames.push_back(tokens[0]);
                        feature->featureType = i;
                        break;
                    }
            }
            feature->xpos = atof(tokens[1].c_str());
            feature->ypos = atof(tokens[2].c_str());
            feature->zpos = atof(tokens[3].c_str());
            feature->rotation = atof(tokens[4].c_str());
            feature->relativeSize = atof(tokens[5].c_str());

            features.push_back(*feature);
        }
        mapFeatureHeader.numFeatureType = featurenames.size();
        printf("INFO: %i Features, %i Types\n", mapFeatureHeader.numFeatures,
                       mapFeatureHeader.numFeatureType );

        header.featurePtr = smf_of.tellp();
        smf_of.seekp(76);
        smf_of.write( (char *)&header.featurePtr, 4);
        smf_of.seekp(header.featurePtr);
        smf_of.write( (char *)&mapFeatureHeader, sizeof( mapFeatureHeader ) );

        vector<string>::iterator i;
        for ( i = featurenames.begin(); i != featurenames.end(); ++i ) {
            string c = *i;
            smf_of.write( c.c_str(), (int)strlen( c.c_str() ) + 1 );
        }

        vector<MapFeatureStruct>::iterator fi;
        for( fi = features.begin(); fi != features.end(); ++fi ) {
            smf_of.write( (char *)&*fi, sizeof( MapFeatureStruct ) );
        }
    }

    smf_of.close();*/

