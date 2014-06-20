#include "config.h"
#include "smt.h"

#include "nvtt_output_handler.h"
#include "dxt1load.h"
#include "util.h"

#include <fstream>
#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>

using namespace std;
OIIO_NAMESPACE_USING;

SMT *
SMT::create( string fileName, bool overwrite, bool verbose, bool quiet )
{
    SMT *smt;
    ifstream file( fileName );
    if( file.good() && !overwrite ) return NULL;
    
    smt = new SMT;
    smt->fileName = fileName;
    smt->reset();
    return smt;
}

SMT *
SMT::open( string fileName, bool verbose, bool quiet )
{
    SMT *smt;
    if( isSMT( fileName ) ){
        smt = new SMT;
        smt->fileName = fileName;
        smt->load();
        return smt;
    }
    return NULL;
}

void
SMT::reset()
{
    if( verbose ) cout << "INFO: Resetting " << fileName;
    // Clears content of SMT file and re-writes the header.
    fstream file( fileName, ios::binary | ios::out );

    if(! file.good() ){
        if(! quiet ) cout << "ERROR: Unable to write to " << fileName << endl;
        return;
    }
    nTiles = 0;
    SMTHeader head;
    head.nTiles = tileSize;
    head.tileType = tileType;

    file.write( (char *)&head, sizeof(SMTHeader) );
    file.flush();
    file.close();
}

void
SMT::setType(int tileType)
{
    this->tileType = tileType;
    tileSize = 0;
    // Calculate the size of the tile data
    // size is the raw format of dxt1 with 4 mip levels
    // DXT1 consists of 64 bits per 4x4 block of pixels.
    // 32x32, 16x16, 8x8, 4x4
    // 512  + 128  + 32 + 8 = 680
    if(tileType == DXT1) {
        int mip = tileRes;
        for( int i=0; i < 4; ++i) {
            tileSize += (mip * mip)/2;
            mip /= 2;
        }
    } else {
        if( !quiet )printf("ERROR: '%i' is not a valid compression type", tileType);
        tileSize = -1;
    }
}

bool
SMT::load()
{
    bool status = false;

    // Test File
    if(! isSMT(fileName) ) {
        if(! quiet ) cout << "ERROR: invalid smt " << fileName << endl;
        return true;
    }

    ifstream inFile;
    inFile.open(fileName, ifstream::in);
    inFile.seekg(0);

    SMTHeader header;
    inFile.read( (char *)&header, sizeof(SMTHeader) );
    inFile.close();

    if( verbose ) {
        cout << "INFO: " << fileName << endl;
        cout << "\tSMT Version: " << header.version << endl;
        cout << "\tTiles: " << header.nTiles << endl;
        cout << "\tTileRes: " << header.tileRes << endl;
        cout << "\tCompression: ";
        if( header.tileType == DXT1 )cout << "dxt1" << endl;
        else {
            cout << "UNKNOWN" << endl;
            status = true;
        }
    }

    this->tileRes = header.tileRes;
    this->nTiles = header.nTiles;
    setType( header.tileType );
    return status;
}

bool
SMT::append( ImageBuf *sourceBuf )
// TODO Code assumes that tiles are DXT1 compressed at this stage,
// abstract the internals out.
{
#ifdef DEBUG_IMG
    static int i = 0;    
    sourceBuf->save( "SMT::append(...)" + to_string(i) + "_0.tif", "tif" );
#endif //DEBUG_IMG

    // Swizzle
    int map[] = { 2, 1, 0, 3 };
    float fill[] = { 0, 0, 0, 255 };
    if( sourceBuf->spec().nchannels < 4 ) map[3] = -1;
    else map[3] = 3;
    ImageBuf fixBuf; 
    ImageBufAlgo::channels( fixBuf, *sourceBuf, 4, map, fill );
    sourceBuf->copy( fixBuf );


    fstream smtFile;
    smtFile.open( this->fileName, ios::binary | ios::out | ios::app );
   
#ifdef DEBUG_IMG
    sourceBuf->save( "SMT::append(...)" + to_string(i) + "_1.tif", "tif" );
#endif //DEBUG_IMG

    unsigned char *std = new unsigned char[tileRes * tileRes * 4];
    sourceBuf->get_pixels(0, tileRes, 0, tileRes, 0, 1, TypeDesc::UINT8, std);

    // process into dds
    nvtt::Compressor compressor;

    nvtt::InputOptions inputOptions;
    inputOptions.setTextureLayout( nvtt::TextureType_2D, this->tileRes, this->tileRes );
    inputOptions.setMipmapData( std, this->tileRes, this->tileRes );

    nvtt::CompressionOptions compressionOptions;
    compressionOptions.setFormat( nvtt::Format_DXT1a );
    if( this->slow_dxt1 ) compressionOptions.setQuality( nvtt::Quality_Normal ); 
    else                  compressionOptions.setQuality( nvtt::Quality_Fastest ); 

    nvtt::OutputOptions outputOptions;

#ifdef DEBUG_IMG    
    outputOptions.setOutputHeader( true );
    outputOptions.setFileName( ("SMT::append(...)" + to_string(i) + "_2.dds").c_str() );
    ++i;

    compressor.process( inputOptions, compressionOptions, outputOptions );
#endif //DEBUG_IMG

    outputOptions.setOutputHeader( false );
    NVTTOutputHandler *nvttHandler = new NVTTOutputHandler( this->tileSize );
    outputOptions.setOutputHandler( nvttHandler );

    compressor.process( inputOptions, compressionOptions, outputOptions );

    smtFile.write( nvttHandler->getBuffer(), this->tileSize );
    smtFile.flush();
    smtFile.close();

    delete nvttHandler;
//    delete tileBuf;

    this->nTiles += 1;

    // retroactively fix up the tile count.
    smtFile.open( this->fileName, ios::binary | ios::in | ios::out );
    smtFile.seekp( 20 );
    smtFile.write( (char *)&(this->nTiles), 4 );
    smtFile.flush();
    smtFile.close();

    return false;
}

ImageBuf *
SMT::getTile(int t)
{
    ImageBuf *imageBuf = NULL; // resulting tile
    ImageSpec imageSpec( tileRes, tileRes, 4, TypeDesc::UINT8 );
    unsigned char *rgba8888; //decompressed rgba8888
    unsigned char *raw_dxt1a; //raw dxt1a data from source file.

    ifstream smtFile( fileName );
    if( smtFile.good() )
    {
        raw_dxt1a = new unsigned char[ tileSize ];

        smtFile.seekg( sizeof(SMTHeader) + tileSize * t );
        smtFile.read( (char *)raw_dxt1a, tileSize );

        rgba8888 = dxt1_load( raw_dxt1a, tileRes, tileRes );
        delete [] raw_dxt1a;

        imageBuf = new ImageBuf( fileName + "_" + to_string(t), imageSpec, rgba8888);
    }
    smtFile.close();

#ifdef DEBUG_IMG
    imageBuf->write("getTile(" + to_string(t) + ").tif", "tif");
#endif //DEBUG_IMG

    return imageBuf;    
}
