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
    smt->verbose = verbose;
    smt->quiet = verbose;
    smt->fileName = fileName;
    smt->reset();
    return smt;
}

SMT *
SMT::open( string fileName, bool verbose, bool quiet )
{
    bool good = false;

    char magic[ 16 ] = "";
    ifstream file( fileName );
    if( file.good() ){
        file.read( magic, 16 );
        if(! strcmp( magic, "spring tilefile" ) ){
            good = true;
            file.close();
        }
    }

    SMT *smt;
    if( good ){
        smt = new SMT;
        smt->verbose = verbose;
        smt->quiet = verbose;
        smt->fileName = fileName;
        smt->load();
        return smt;
    }
    return NULL;
}

void
SMT::reset()
{
    if( verbose ) cout << "INFO: Resetting " << fileName << endl;
    // Clears content of SMT file and re-writes the header.
    fstream file( fileName, ios::binary | ios::out );

    if(! file.good() ){
        if(! quiet ) cout << "ERROR: Unable to write to " << fileName << endl;
        return;
    }

    //re write header
    SMTHeader head;
    head.tileType = tileType;
    head.tileRes = tileRes;

    file.write( (char *)&head, sizeof(SMTHeader) );
    file.flush();
    file.close();

    // fix up local attributes
    nTiles = 0;
    calcTileSize();   
}

void
SMT::setType(unsigned int t)
{
    if( tileType == t)return;
    tileType = t;
    reset();
}

void
SMT::setTileRes(unsigned int r)
{
    if( tileRes == r )return;
    tileRes = r;
    reset();
}

void
SMT::calcTileSize()
{
    // size is the raw format of dxt1 with 4 mip levels
    // DXT1 consists of 64 bits per 4x4 block of pixels.
    // 32x32, 16x16, 8x8, 4x4
    // 512  + 128  + 32 + 8 = 680
    tileSize = 0;
    if(tileType == DXT1) {
        int mip = tileRes;
        for( int i=0; i < 4; ++i) {
            tileSize += (mip * mip)/2;
            mip /= 2;
        }
    }
}

void
SMT::load()
{
    SMTHeader header;

    ifstream inFile(fileName, ifstream::in);
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
        }
    }

    tileRes  = header.tileRes;
    nTiles   = header.nTiles;
    tileType = header.tileType;
    calcTileSize();
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
   
#ifdef DEBUG_IMG
    sourceBuf->save( "SMT::append(...)" + to_string(i) + "_1.tif", "tif" );
#endif //DEBUG_IMG

    // Get raw data
    unsigned char *std = new unsigned char[tileRes * tileRes * 4];
    sourceBuf->get_pixels(0, tileRes, 0, tileRes, 0, 1, TypeDesc::UINT8, std);

    // process into dds
    nvtt::Compressor compressor;

    nvtt::InputOptions inputOptions;
    inputOptions.setTextureLayout( nvtt::TextureType_2D, tileRes, tileRes );
    inputOptions.setMipmapData( std, tileRes, tileRes );

    nvtt::CompressionOptions compressionOptions;
    compressionOptions.setFormat( nvtt::Format_DXT1a );
    if( slow_dxt1 ) compressionOptions.setQuality( nvtt::Quality_Normal ); 
    else                  compressionOptions.setQuality( nvtt::Quality_Fastest ); 

    nvtt::OutputOptions outputOptions;

#ifdef DEBUG_IMG    
    outputOptions.setOutputHeader( true );
    outputOptions.setFileName( ("SMT::append(...)" + to_string(i) + "_2.dds").c_str() );
    ++i;

    compressor.process( inputOptions, compressionOptions, outputOptions );
#endif //DEBUG_IMG

    outputOptions.setOutputHeader( false );
    NVTTOutputHandler *nvttHandler = new NVTTOutputHandler( tileSize );
    outputOptions.setOutputHandler( nvttHandler );

    compressor.process( inputOptions, compressionOptions, outputOptions );

    // save to file
    fstream smtFile(fileName, ios::binary | ios::in | ios::out);

    smtFile.seekp( sizeof(SMTHeader) + tileSize * nTiles );
    smtFile.write( nvttHandler->getBuffer(), tileSize );

    ++nTiles;

    smtFile.seekp( 20 );
    smtFile.write( (char *)&(nTiles), 4 );

    smtFile.flush();
    smtFile.close();

    // fix internal state

    // cleanup
    delete nvttHandler;
    return false;
}

ImageBuf *
SMT::getTile(int n)
{
    ImageBuf *imageBuf = NULL; // resulting tile
    ImageSpec imageSpec( tileRes, tileRes, 4, TypeDesc::UINT8 );
    unsigned char *rgba8888; //decompressed rgba8888
    unsigned char *raw_dxt1a; //raw dxt1a data from source file.

    ifstream smtFile( fileName );
    if( smtFile.good() )
    {
        raw_dxt1a = new unsigned char[ tileSize ];

        smtFile.seekg( sizeof(SMTHeader) + tileSize * n );
        smtFile.read( (char *)raw_dxt1a, tileSize );

        rgba8888 = dxt1_load( raw_dxt1a, tileRes, tileRes );
        delete [] raw_dxt1a;

        imageBuf = new ImageBuf( fileName + "_" + to_string(n), imageSpec, rgba8888);
    }
    smtFile.close();

#ifdef DEBUG_IMG
    imageBuf->write("getTile(" + to_string(n) + ").tif", "tif");
#endif //DEBUG_IMG

    return imageBuf;    
}
