#pragma once

#include <OpenImageIO/imagebuf.h>

#include "smt.h"
#include "tilecache.h"
#include "tilemap.h"


namespace SMF_Tools
{
    extern bool  verbose, quiet;
    extern float cpet;
    extern int cnet, cnum;

    OpenImageIO::ImageBuf *reconstruct( TileCache &cache, TileMap *tileMap );
    OpenImageIO::ImageBuf *collate( TileCache &cache,
            unsigned int hstride = 0,
            unsigned int vstride = 0 );
    OpenImageIO::ImageBuf *openTilemap( std::string filename );

    bool consolidate( SMT *smt, TileCache &cache, OpenImageIO::ImageBuf * tilemap);
    void imageToSMT( SMT *smt, OpenImageIO::ImageBuf *image );

}