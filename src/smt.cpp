#include "config.h"
#include "smt.h"

#include "util.h"

#include <fstream>
#include <squish.h>
#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>

using namespace std;
OIIO_NAMESPACE_USING;

SMT *SMT::create( string fileName, bool overwrite, bool verbose, bool quiet, bool dxt1_quality ){
    SMT *smt;
    ifstream file( fileName );
    if( file.good() && !overwrite ) return NULL;
    
    smt = new SMT( fileName, verbose, quiet, dxt1_quality );
    smt->init = !smt->reset();
    return smt;
}

SMT * SMT::open( string fileName, bool verbose, bool quiet, bool dxt1_quality ){
    bool good = false;

    char magic[ 16 ] = "";
    ifstream file( fileName );
    if( file.good() ){
        file.read( (char *)magic, 16 );
        if(! strcmp( magic, "spring tilefile" ) ){
            good = true;
            file.close();
        }
    }

    SMT *smt;
    if( good ){
        smt = new SMT( fileName, verbose, quiet, dxt1_quality );
        smt->init = !smt->load();
        return smt;
    }
    return NULL;
}

bool SMT::reset(){
    if( verbose ) cout << "INFO: Resetting " << fileName << endl;
    // Clears content of SMT file and re-writes the header.
    fstream file( fileName, ios::binary | ios::out );

    if(! file.good() ){
        if(! quiet ) cout << "ERROR: Unable to write to " << fileName << endl;
        return true;
    }

    //reset tile count
    header.nTiles = 0;

    // write header
    file.write( (char *)&header, sizeof(SMT::Header) );
    file.flush();
    file.close();

    return false;   
}

void SMT::setType( TileType t ){
    if( header.tileType == t)return;
    header.tileType = t;
    reset();
}

void SMT::setTileRes( int r ){
    if( header.tileRes == r )return;
    header.tileRes = r;
    reset();
}

/*! Calculate the size of the raw format of dxt1 with 4 mip levels
 * DXT1 consists of 64 bits per 4x4 block of pixels.
 * 32x32, 16x16, 8x8, 4x4
 * 512  + 128  + 32 + 8 = 680
 */
void SMT::calcTileSize() {
    tileSize = 0;
    if(header.tileType == TileType::DXT1) {
        int mip = header.tileRes;
        for( int i=0; i < 4; ++i) {
            tileSize += (mip * mip)/2;
            mip /= 2;
        }
    }
}

bool SMT::load() {
    ifstream inFile(fileName, ifstream::in);
    inFile.read( (char *)&header, sizeof(SMT::Header) );
    inFile.close();

    if( verbose ) {
        cout << "INFO: " << fileName << endl;
        cout << "\tSMT Version: " << header.version << endl;
        cout << "\tTiles: " << header.nTiles << endl;
        cout << "\tTileRes: " << header.tileRes << endl;
        cout << "\tCompression: ";
        if( header.tileType == TileType::DXT1 )cout << "dxt1" << endl;
        else {
            cout << "UNKNOWN" << endl;
        }
    }

    calcTileSize();
    return false;
}

/*! Append tiles to the end of the SMT file and update the count
 * TODO Code assumes that tiles are DXT1 compressed at this stage,
 * TODO abstract the internals out.
 */
bool SMT::append( ImageBuf *sourceBuf ){
#ifdef DEBUG_IMG
    static int i = 0;    
    sourceBuf->save( "SMT::append_sourceBuf_" + to_string(i) + "_0.tif", "tif" );
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
    sourceBuf->save( "SMT::append_sourceBuf_" + to_string(i) + "_1_swizzle.tif", "tif" );
#endif //DEBUG_IMG

    // Get raw data
    unsigned char *std = new unsigned char[header.tileRes * header.tileRes * 4];
    if(! sourceBuf->get_pixels(0, header.tileRes,
                0, header.tileRes, 0, 1, TypeDesc::UINT8, std) ){
        cout << "failed to get pixels from buffer" << endl;
        return true;
    }


    // save to file
    fstream smtFile(fileName, ios::binary | ios::in | ios::out);

    smtFile.seekp( sizeof(SMT::Header) + tileSize * header.nTiles );
//    smtFile.write( nvttHandler->getBuffer(), tileSize );

    ++header.nTiles;

    smtFile.seekp( 20 );
    smtFile.write( (char *)&(header.nTiles), 4 );

    smtFile.flush();
    smtFile.close();

    return false;
}

ImageBuf *SMT::getTile( int n ){
    ImageBuf *imageBuf = NULL; // resulting tile
    ImageSpec imageSpec( header.tileRes, header.tileRes, 4, TypeDesc::UINT8 );
    unsigned char *rgba8888; //decompressed rgba8888
    unsigned char *raw_dxt1a; //raw dxt1a data from source file.

    ifstream smtFile( fileName );
    if( smtFile.good() )
    {
        raw_dxt1a = new unsigned char[ tileSize ];

        smtFile.seekg( sizeof(SMT::Header) + tileSize * n );
        smtFile.read( (char *)raw_dxt1a, tileSize );

//FIXME        rgba8888 = dxt1_load( raw_dxt1a, header.tileRes, header.tileRes );
        delete [] raw_dxt1a;

        imageBuf = new ImageBuf( fileName + "_" + to_string(n), imageSpec, rgba8888);
    }
    smtFile.close();

#ifdef DEBUG_IMG
    imageBuf->write("getTile(" + to_string(n) + ").tif", "tif");
#endif //DEBUG_IMG

    return imageBuf;    
}
