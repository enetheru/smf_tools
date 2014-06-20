#ifndef SMTOOL_H
#define SMTOOL_H

#include "smt.h"
#include "tilecache.h"

OIIO_NAMESPACE_USING
using namespace std;

namespace SMTool
{
    extern bool  verbose, quiet;
    extern float cpet;
    extern int cnet, cnum;

    ImageBuf *reconstruct( TileCache &, ImageBuf *tilemapBuf );
    ImageBuf *collate( TileCache &cache, unsigned int hstride = 0, unsigned int vstride = 0 );
    ImageBuf *openTilemap( string filename );
    bool create( string fileName, bool overwrite=false );
    bool consolidate( SMT &smt, TileCache &cache, ImageBuf * tilemap);
    void imageToSMT( ImageBuf *buf, SMT &smt);

}

#endif //SMTOOL_H
