#include "config.h"
#include "smt.h"

#include "util.h"

#include "elog/elog.h"
#include <squish.h>
#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <fstream>

using namespace std;
OIIO_NAMESPACE_USING;

SMT *
SMT::create( string fileName, bool overwrite, bool dxt1_quality )
{
    SMT *smt;
    ifstream file( fileName );
    if( file.good() && !overwrite ) return NULL;
    
    smt = new SMT( fileName, dxt1_quality );
    smt->reset();
    return smt;
}

SMT *
SMT::open( string fileName )
{
    bool good = false;

    char magic[ 16 ] = "";
    ifstream file( fileName );
    if( file.good() ){
        file.read( (char *)magic, 16 );
        if(! strcmp( magic, "spring tilefile" ) ){
            good = true;
            file.close();
        }
        else {
            LOG( ERROR ) << fileName << " is not an SMT file.";
        }
    }

    SMT *smt;
    if( good ){
        smt = new SMT( fileName );
        smt->load();
        return smt;
    }
    return NULL;
}

void
SMT::reset( )
{
    LOG(INFO) << "Resetting " << fileName;
    // Clears content of SMT file and re-writes the header.
    init = false;
    fstream file( fileName, ios::binary | ios::out );

    if(! file.good() ){
        LOG(ERROR) << "Unable to write to " << fileName;
        return;
    }

    //reset tile count
    header.nTiles = 0;

    // write header
    file.write( (char *)&header, sizeof(SMT::Header) );
    file.flush();
    file.close();
    init = true;
}

void
SMT::setType( TileType t )
{
    if( header.tileType == t)return;
    header.tileType = t;
    reset();
}

void
SMT::setTileSize( uint32_t r )
{
    if( header.tileSize == r )return;
    header.tileSize = r;
    reset();
}

void
SMT::calcTileBytes( )
{
    tileBytes = 0;
    if(header.tileType == TileType::DXT1) {
        int mip = header.tileSize;
        for( int i=0; i < 4; ++i) {
            tileBytes += (mip * mip)/2;
            mip /= 2;
        }
    }
}

void
SMT::load( )
{
    ifstream inFile(fileName, ifstream::in);
    inFile.read( (char *)&header, sizeof(SMT::Header) );
    inFile.close();
    calcTileBytes();
    init = true;
}

std::string
SMT::info( )
{
    stringstream ss;
    ss <<  "\tFilename: " << fileName << endl
        << "\tVersion: " << header.version << endl
        << "\tTiles: " << header.nTiles << endl
        << "\tTileSize: " << header.tileSize << "x" << header.tileSize << endl
        << "\tCompression: ";
    if( header.tileType == TileType::DXT1 ) ss << "dxt1" << endl;
    else {
        ss << "UNKNOWN" << endl;
    }
    return ss.str();
}

/*! Append tiles to the end of the SMT file and update the count
 * TODO Code assumes that tiles are DXT1 compressed at this stage,
 * TODO abstract the internals out.
 */
void
SMT::append( ImageBuf *sourceBuf )
{
#ifdef DEBUG_IMG
    static int i = 0;    
    sourceBuf->save( "SMT::append_sourceBuf_" + to_string(i) + "_0.tif", "tif" );
#endif //DEBUG_IMG

    // Swizzle
    int map[] = { 2, 1, 0, 3 };
    float fill[] = { 0, 0, 0, 255 };
    if( sourceBuf->spec().nchannels < 4 ) map[3] = -1;
    else map[3] = 3;
    ImageBuf *tempBufa = new ImageBuf; 
    ImageBufAlgo::channels( *tempBufa, *sourceBuf, 4, map, fill );
   
#ifdef DEBUG_IMG
    sourceBuf->save( "SMT::append_sourceBuf_" + to_string(i) + "_1_swizzle.tif", "tif" );
#endif //DEBUG_IMG

    ImageSpec spec;
    ImageBuf *tempBufb = NULL;
    int blocks_size = 0;
    squish::u8 *blocks = NULL;
    fstream file(fileName, ios::binary | ios::in | ios::out);
    file.seekp( sizeof(SMT::Header) + tileBytes * header.nTiles );
    for( int i = 0; i < 4; ++i ){
        spec = tempBufa->specmod();

        blocks_size = squish::GetStorageRequirements(
                spec.width, spec.height, squish::kDxt1 );

        if(! blocks ) blocks = new squish::u8[ blocks_size ];

        squish::CompressImage( (squish::u8 *)tempBufa->localpixels(),
                spec.width, spec.height, blocks, squish::kDxt1 );

        // Write data to smf
        file.write( (char*)blocks, blocks_size );

        spec.width = spec.width >> 1;
        spec.height = spec.height >> 1;
        tempBufb = tempBufa;
        tempBufa = scale( tempBufb, spec );
        delete tempBufb;
    }
    delete blocks;
    delete tempBufa;

    ++header.nTiles;

    file.seekp( 20 );
    file.write( (char *)&(header.nTiles), 4 );

    file.flush();
    file.close();
}

ImageBuf *
SMT::getTile( uint32_t n )
{
    ImageBuf *imageBuf = NULL;
    ImageSpec imageSpec( header.tileSize, header.tileSize, 4, TypeDesc::UINT8 );

    char *raw_dxt1a = new char[ tileBytes ];
    char *rgba8888 = new char[ header.tileSize * header.tileSize * 4 ];

    ifstream file( fileName );
    if( file.good() )
    {

        file.seekg( sizeof(SMT::Header) + tileBytes * n );
        file.read( (char *)raw_dxt1a, tileBytes );

        squish::DecompressImage( (squish::u8 *)rgba8888,
                header.tileSize, header.tileSize, raw_dxt1a, squish::kDxt1 );

        imageBuf = new ImageBuf( fileName + "_" + to_string(n), imageSpec, rgba8888);
    }
    delete [] raw_dxt1a;
    file.close();

#ifdef DEBUG_IMG
    imageBuf->write("getTile(" + to_string(n) + ").tif", "tif");
#endif //DEBUG_IMG

    return imageBuf;    
}
