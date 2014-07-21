#ifndef SMTOOL_H
#define SMTOOL_H

#include "smt.h"
#include "tilecache.h"
#include "tilemap.h"

OIIO_NAMESPACE_USING
using namespace std;

namespace SMTool
{
    extern bool  verbose, quiet;
    extern float cpet;
    extern int cnet, cnum;

    ImageBuf *reconstruct( TileCache &cache, TileMap *tileMap );
    ImageBuf *collate( TileCache &cache,
            unsigned int hstride = 0,
            unsigned int vstride = 0 );
    ImageBuf *openTilemap( string filename );

    bool consolidate( SMT *smt, TileCache &cache, ImageBuf * tilemap);
    void imageToSMT( SMT *smt, ImageBuf *image );

}

#endif //SMTOOL_H
