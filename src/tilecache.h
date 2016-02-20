#pragma once

#include <vector>
#include <string>

#include <OpenImageIO/imagebuf.h>

class TileCache
{
    // member data
    uint32_t nTiles = 0;
	// FIXME this twin vector mapping can be turned into a pair or a tuple
    std::vector< uint32_t > map;
    std::vector< std::string > fileNames;

public:
    // modifications
    void addSource( const std::string );

    // data access
    uint32_t getNTiles ( ){ return nTiles; };;

	/// get the original tile from the cache
	/*  TODO
	 *
	 */
	std::unique_ptr< OpenImageIO::ImageBuf > getOriginal( const uint32_t n );

	/// gets the tile but modified to match specifications
	/*  TODO
	 *
	 */
    std::unique_ptr< OpenImageIO::ImageBuf > getSpec(
			const uint32_t n, const OpenImageIO::ImageSpec & );

	/// the same as get original but using bracket operator
	/*  TODO
	 *
	 */
    std::unique_ptr< OpenImageIO::ImageBuf > operator() ( const uint32_t n )
			{ return getOriginal( n ); }

};
