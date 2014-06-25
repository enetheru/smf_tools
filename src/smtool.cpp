#include "smtool.h"

#include "config.h"
#include "smt.h"

#include <fstream>
#include <sys/time.h>
#include <deque>

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include "nvtt_output_handler.h"
#include "smf.h"
#include "dxt1load.h"
#include "util.h"

namespace SMTool
{
    bool verbose = false;
    bool quiet = false;
    float cpet;
    int cnet, cnum;
}

ImageBuf *
SMTool::reconstruct( TileCache &cache, ImageBuf *map)
{
    if( verbose )cout << "INFO: Reconstructing Big\n";
    if(! map )return NULL;
    ImageSpec spec = map->spec();

    // TODO use a pixel iterator instead of raw access.
    unsigned int *tilemap = new unsigned int [ spec.width * spec.height ];
    map->get_pixels( 0, spec.width, 0, spec.height, 0, 1, TypeDesc::UINT, tilemap );

    // allocate enough data for our large image
    if( verbose ) cout << "\tGenerating " << spec.width * cache.getTileRes() << "x"
        << spec.height * cache.getTileRes() << endl;

    ImageSpec bigSpec( spec.width * cache.getTileRes(),
            spec.height * cache.getTileRes(), 4, TypeDesc::UINT8 );
    ImageBuf *bigBuf = new ImageBuf( "big", bigSpec );

    // Loop through tile index
    ImageBuf *tileBuf = NULL;
    for( int y = 0; y < spec.height; ++y ) {
        for( int x = 0; x < spec.width; ++x ) {
            unsigned int tilenum = tilemap[ y * spec.width + x ];

            tileBuf = cache.getTile( tilenum );
            if(! tileBuf ) continue;

            unsigned int xbegin = cache.getTileRes() * x;
            unsigned int ybegin = cache.getTileRes() * y;
            ImageBufAlgo::paste(*bigBuf, xbegin, ybegin, 0, 0, *tileBuf);

            delete tileBuf;
            if( verbose )printf("\033[0G\tProcessing tile %i of %i.",
                y * spec.width + x, spec.width * spec.height );
        }
    }
    cout << endl;

    delete map;
    delete tilemap;
    return bigBuf;    
}

ImageBuf *
SMTool::collate( TileCache &cache, unsigned int hstride, unsigned int vstride )
{
    if( verbose )cout << "INFO: Collating Big\n";
    // OpenImageIO
    ImageBuf *tileBuf = NULL;

    // Allocate Image size large enough to accomodate the tiles,
    if(! hstride && ! vstride ) hstride = vstride = ceil( sqrt( cache.getNTiles() ) );
    else if(! hstride ) hstride = ceil( (float)cache.getNTiles() / (float)vstride);
    else if(! vstride ) vstride = ceil( (float)cache.getNTiles() / (float)hstride);

    ImageSpec bigSpec(
            hstride * cache.getTileRes(),
            vstride * cache.getTileRes(),
            4, TypeDesc::UINT8 );

    ImageBuf *bigBuf = new ImageBuf( "big", bigSpec);
    
    if( verbose ){
        cout << "\tImage created: " << bigSpec.width << "x" << bigSpec.height << endl;
    }
    // Loop through tiles copying the data to our image buffer
    unsigned int limit = fmin( hstride * vstride, cache.getNTiles() );
    for(unsigned int i = 0; i < limit; ++i ) {
        // Pull data
        tileBuf = cache.getTile( i );

        unsigned int dx = cache.getTileRes() * (i % hstride);
        unsigned int dy = cache.getTileRes() * (i / hstride);

        ImageBufAlgo::paste(*bigBuf, dx, dy, 0, 0, *tileBuf);

        delete tileBuf;
    //    if( verbose )printf("\033[0G\tProcessing tile %i of %i.",
    //        i+1, limit );
    }
    cout << endl;
    return bigBuf;
}

ImageBuf *
SMTool::openTilemap( string fileName )
{
    ImageBuf *buf = NULL;
    ImageSpec spec;
    //Load tilemap from SMF
    if( isSMF( fileName ) ){
        SMF smfFile( fileName );
        buf = smfFile.getTilemap();
    }

    // Else load tilemap from image
    if(! buf ){
        buf = new ImageBuf( fileName );
        buf->read( 0, 0, true, TypeDesc::UINT );

        if(! buf->initialized() ){
            delete buf;
            if(! quiet ) cout << "ERROR: cannot load " <<  fileName << endl;
        }
    }

    if(! buf )return NULL;
    //TODO Else load tilemap from csv

    spec = buf->spec();
    if( verbose ){
        cout << "INFO: " << fileName << endl;
        cout << "\tSize: " << spec.width << "x" << spec.height <<  endl;
    }

    ImageBuf fix;
    if( spec.nchannels != 1 )
    {
        int map[] = {0};
        ImageBufAlgo::channels( fix, *buf, 1, map );
        buf->copy( fix );
        fix.clear();
    }

    return buf;
}

bool
SMTool::consolidate(SMT *smt, TileCache &cache, ImageBuf *tilemap)
{
    // TODO write consolidation method
    return false;
}


class TileBufListEntry {
public:
    ImageBuf image;
    ImageBufAlgo::PixelStats stats;
    int tileNum;
};

void
SMTool::imageToSMT( SMT *smt, ImageBuf *sourceBuf )
{
    if( verbose )
        cout << "INFO: Converting image to tiles and saving to smt" << endl;
    
    ImageSpec sourceSpec = sourceBuf->spec();
    unsigned int tileRes = smt->getTileRes();

    ImageBuf tileBuf;
    ImageSpec tileSpec( tileRes, tileRes, 4, TypeDesc::UINT8 );
    ROI roi( 0, 1, 0, 1, 0, 1, 0, 4 );

    ImageSpec mapSpec(
            sourceSpec.width / tileSpec.width,
            sourceSpec.height / tileSpec.height,
            1,
            TypeDesc::UINT );

    ImageBuf mapBuf( "tilemap", mapSpec );

    // Time reporting vars
    // FIXME simplify
    timeval t1, t2;
    double elapsedTime;
    deque<double> readings;
    double averageTime = 0;
    double intervalTime = 0;

    // Loop vars
    unsigned int currentTile;

    // Comparison vars
    bool match;
    unsigned int i;
    string hash;
    vector<string> hashTable;
    TileBufListEntry *listEntry;
    deque<TileBufListEntry *> tileList;

    if( verbose ){
        cout << "\tSource: " << sourceSpec.width << "x" << sourceSpec.height << endl;
        cout << "\ttileRes: " << tileSpec.width << "x" << tileSpec.height << endl;
        cout << "\ttilemap: " << mapSpec.width << "x" << mapSpec.height << endl;
        cout << "\tmaxTiles: " << mapSpec.image_pixels() << endl;
        cout << "  Processing tiles:\n";
    }

    for( ImageBuf::Iterator<unsigned int, unsigned int> it(mapBuf); ! it.done(); ++it ){
        gettimeofday(&t1, NULL);

        currentTile = it.y() * mapSpec.width + it.x();

        // define the cropping region.
        roi.xbegin = it.x() * tileSpec.width;
        roi.xend = it.x() * tileSpec.width + tileSpec.width;
        roi.ybegin = it.y() * tileSpec.height;
        roi.yend = it.y() * tileSpec.height + tileSpec.height;
//        cout << roi.xbegin << " " << roi.xend << " " << roi.ybegin << " " << roi.yend << endl;

        // crop out tile.
        ImageBufAlgo::cut( tileBuf, *sourceBuf, roi );

#ifdef DEBUG_IMG
        tileBuf.save("SMTool::imageToSMT_tileBuf_" + to_string( currentTile + 1 ) + ".tif", "tif");
#endif //DEBUG_IMG

        // reset match variables
        match = false;
        i = smt->getNTiles();

        // Optimisation
        if( cnum == 0){
            // only exact matches will be referenced.
            hash = ImageBufAlgo::computePixelHashSHA1( tileBuf );
            for( i = 0; i < hashTable.size(); ++i ){
                if(! hashTable[ i ].compare( hash ) ){
                    match = true;
                    break;
                } 
            }
            if(! match ) hashTable.push_back( hash );
        }
        else {
            //Comparison based on numerical differences of pixels
            listEntry = new TileBufListEntry;
            listEntry->image.copy( tileBuf );
            listEntry->tileNum = smt->getNTiles();

            ImageBufAlgo::CompareResults result;

            for( auto it = tileList.begin(); it != tileList.end(); it++ ){
                TileBufListEntry *listEntry2 = *it;
                ImageBufAlgo::compare( tileBuf, listEntry2->image,
                        cpet, 1.0f, result);
                //TODO give control on tweaking matching
                if( (int)result.nfail < cnet ){
                    match = true;
                    i = listEntry2->tileNum;
                    delete listEntry;
                    break;
                }
            }
            if(! match ){
                tileList.push_back( listEntry );
                if( (int)tileList.size() > cnum ){
                    delete tileList[ 0 ];
                    tileList.pop_front();
                }
            }
        }

        // write tile to file.
        if(! match ) {
            smt->append( &tileBuf );
        }

        tileBuf.clear();

        // Write index to tilemap
        it[0] = currentTile;

        // progress report
        gettimeofday( &t2, NULL );
        // compute and print the elapsed time in millisec
        elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
        elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
        readings.push_back( elapsedTime );
        if( readings.size() > 1000 ) readings.pop_front();
        intervalTime += elapsedTime;
        if( verbose && intervalTime > 1 ){
            for( auto i = readings.begin(); i != readings.end(); ++i )
                averageTime += *i;
            averageTime /= readings.size();
            intervalTime = 0;
            printf("\033[0G\t%u of %llu %%%0.1f complete | %%%0.1f savings | %0.1fs remaining.",
                    currentTile+1, mapSpec.image_pixels(),
                    (float)currentTile+1 / mapSpec.image_pixels() * 100,
                (float)(1 - (float)smt->getNTiles() / (float)currentTile+1) * 100,
                averageTime * (mapSpec.image_pixels() - currentTile+1) / 1000);
        }
    }
    hashTable.clear();
    if( verbose ) cout << endl;

    // Save tileindex
    mapBuf.save( "tilemap.exr", "exr" );
}


/*
struct coord {
    int x,y;
};

struct type {
    string imageName;
    vector<coord> coordinates;
};

void
SMTUtil::pasteDecals(ImageBuf *bigBuf)
{
    printf("WARNING: decal pasting is not yet implemented\n");
    return;
    // load definitions file
    vector<type> list;

    // Parse the file
    cout << "INFO: Reading " << decalFN << endl;
    ifstream decalList( decalFN );

    string line;
    string cell;
    vector<string> tokens;
    while ( getline( decalList, line ) ) {
        stringstream lineStream( line );
        tokens.clear();
        while( getline( lineStream, cell, ',' ) ) tokens.push_back( cell );

        if(tokens.size() <2 ) continue;

        // use parsed line
        coord xy;
        type *decal;
        // search for existing decal filename
        bool found = false;
        for(unsigned int i = 0; i < list.size(); ++i ) {
            if( !list[i].imageName.compare( tokens[0].c_str() ) ) {
                decal = &list[i];
                found = true;
                break;
            }
        }
        if( !found ) {
            decal = new type;
            decal->imageName = tokens[0].c_str();
        }
        xy.x = atoi( tokens[1].c_str() );
        xy.y = atoi( tokens[2].c_str() );
        decal->coordinates.push_back(xy);
        if( !found ) list.push_back(*decal);
    }

    // loop through list of decals.
    for(unsigned int i = 0; i < list.size(); ++i ) {
        if( verbose )printf("     %i %s ", (int)list[i].coordinates.size(),
            list[i].imageName.c_str() );

        ImageBuf decalBuf( list[i].imageName );
        decalBuf.read( 0, 0, false, TypeDesc::FLOAT );
        if( !decalBuf.initialized() ) {
            printf("ERROR: could not load %s\n", list[i].imageName.c_str() );
            continue;
        }
        ImageSpec decalSpec = decalBuf.spec();

        printf("(%i,%i)%i\n", decalSpec.width, decalSpec.height,
            decalSpec.nchannels );

        // loop through list of coordinates
        for(unsigned int j = 0; j < list[i].coordinates.size(); ++j ) {
            coord xy = list[i].coordinates[j];

            ROI roi( xy.x, xy.x + decalSpec.width,
                    xy.y, xy.y + decalSpec.height,
                    0,1,
                    0,5);
            ImageBuf pasteBuf;
            ImageBufAlgo::crop(pasteBuf, *bigBuf, roi);

            ImageBuf compBuf;
            if( !ImageBufAlgo::over( compBuf, decalBuf, pasteBuf) ) {
                cout << "ERROR with composite: ";
                cout << compBuf.geterror() << endl;
                continue;
            }
            char filename[256];
            sprintf(filename, "test%i.png", j);
            compBuf.save(filename);
        }
    }
    return;
}
*/

