#include <string>

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>

#include "elog/elog.h"

#include "util.h"
#include "smt.h"
#include "smf.h"
#include "tilecache.h"

std::unique_ptr< OpenImageIO::ImageBuf >
//FIXME remove the 2 once all is said and done
TileCache::getOriginal( const uint32_t n )
{
	std::unique_ptr< OpenImageIO::ImageBuf >
		outBuf( new OpenImageIO::ImageBuf );
    if( n >= nTiles ) return outBuf;

    SMT *smt = nullptr;
    static SMT *lastSmt = nullptr;

    auto i = map.begin();
    auto fileName = fileNames.begin();
    while( *i <= n ) { ++i; ++fileName; }

	// already open smt file?
    if( lastSmt && (! lastSmt->fileName.compare( *fileName )) ){
        outBuf = lastSmt->getTile( n - *i + lastSmt->nTiles);
    	return std::move( outBuf );
    }
	// open a new smt file?
    if( (smt = SMT::open( *fileName )) ){
		//FIXME shouldnt have a manual delete here, use move scemantics instead
        delete lastSmt;
        lastSmt = smt;
        outBuf = lastSmt->getTile( n - *i + lastSmt->nTiles );
    	return std::move( outBuf );
    }
	// open the image file?
	outBuf->reset( *fileName );
	if( outBuf->initialized() ) {
		return std::move( outBuf );
	}

	LOG( ERROR ) << "failed to open source for tile: " << n;
    return std::move( outBuf );

}

std::unique_ptr< OpenImageIO::ImageBuf >
TileCache::getSpec( uint32_t n, const OpenImageIO::ImageSpec &spec )
{
	OIIO_NAMESPACE_USING;
    CHECK( spec.width     ) << "TileCache::getSpec, cannot request zero width";
    CHECK( spec.height    ) << "TileCache::getSpec, cannot request zero height";
    CHECK( spec.nchannels ) << "TileCache::getSpec, cannot request zero channels";

	std::unique_ptr< OpenImageIO::ImageBuf > outBuf( getOriginal( n ) );
	CHECK( outBuf ) << "nullptr gotten from getOriginal(" << n << ")";

    outBuf = fix_channels( std::move( outBuf ), spec );
    outBuf = fix_format(   std::move( outBuf ), spec );
    outBuf = fix_scale(    std::move( outBuf ), spec );

    return std::move( outBuf );
}

//TODO go over this function to see if it can be refactored
void
TileCache::addSource( const std::string fileName )
{
	OIIO_NAMESPACE_USING;
    ImageInput *image = nullptr;
    if( (image = ImageInput::open( fileName )) ){
        image->close();
        delete image;

        nTiles++;
        map.push_back( nTiles );
        fileNames.push_back( fileName );
        return;
    }

    SMT *smt = nullptr;
    if( (smt = SMT::open( fileName )) ){
        if(! smt->nTiles ) return;
        nTiles += smt->nTiles;
        map.push_back( nTiles );
        fileNames.push_back( fileName );

        delete smt;
        return;
    }

    SMF *smf = nullptr;
    if( (smf = SMF::open( fileName )) ){
        // get the fileNames here
        auto smtList = smf->getSMTList();
        for( auto i : smtList ) addSource( i.second );
        delete smf;
        return;
    }

    LOG(ERROR) << "unrecognised format: " << fileName;
}


