//
// Created by nicho on 20/11/2023.
//

#include "util_oiio.h"

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>


std::unique_ptr< OIIO::ImageBuf >
fix_channels( std::unique_ptr< OIIO::ImageBuf> && inBuf, const OIIO::ImageSpec &spec ) {
    OIIO_NAMESPACE_USING

    int map[] = { 0, 1, 2, 3 };
    static const float fill[] = { 0, 0, 0, 1.0 };

    if( !inBuf ){
        SPDLOG_CRITICAL( "nullptr passed to fix_channels()" );
        //FIXME I think I should be able to assume that nullptr is not passed in here.
        return nullptr;
    }

    // return a copy of the original if it's the correct size.
    if( inBuf->spec().nchannels == spec.nchannels ) return std::move( inBuf );
    if( inBuf->spec().nchannels < 4 ) map[3] = -1;
    if( inBuf->spec().nchannels < 3 ) map[2] = -1;
    if( inBuf->spec().nchannels < 2 ) map[1] = -1;

    // otherwise, update channels to spec channels
    std::unique_ptr< OIIO::ImageBuf > outBuf( new OIIO::ImageBuf );
    ImageBufAlgo::channels( *outBuf, *inBuf, spec.nchannels, map, fill );
    return outBuf;
}

OIIO::ImageBuf
channels( const OIIO::ImageBuf &sourceBuf, const OIIO::ImageSpec &destSpec )
{
    OIIO_NAMESPACE_USING
    int map[] = { 0, 1, 2, 3 };
    float fill[] = { 0, 0, 0, 1.0 };

    if( !sourceBuf.initialized() ) {
        SPDLOG_CRITICAL("sourceBuf is not initialised");
        return {};
    }

    // return a copy of the original if it's the correct size.
    if( sourceBuf.spec().nchannels == destSpec.nchannels ) return {};
    if( sourceBuf.spec().nchannels < 4 ) map[3] = -1;
    if( sourceBuf.spec().nchannels < 3 ) map[2] = -1;
    if( sourceBuf.spec().nchannels < 2 ) map[1] = -1;

    // otherwise, update channels to spec channels
    return ImageBufAlgo::channels( sourceBuf, destSpec.nchannels, map, fill );
}

std::unique_ptr< OIIO::ImageBuf >
fix_scale(
    std::unique_ptr< OIIO::ImageBuf> && inBuf,
    const OIIO::ImageSpec &spec )
{
    OIIO_NAMESPACE_USING
    using std::unique_ptr;

    if( !inBuf ) {
        SPDLOG_CRITICAL("nullptr passed to fix_scale()" );
        //FIXME I think I should be able to assume that nullptr is not passed in here.
        return nullptr;
    }

    // return the inBuf if no change is required.
    if( (inBuf->spec().width  == spec.width )
     && (inBuf->spec().height == spec.height) ){
        SPDLOG_INFO( "no scale required" );
        return std::move( inBuf );
    }

    // Otherwise scale
    ROI roi(0, spec.width, 0, spec.height, 0, 1, 0, inBuf->spec().nchannels );
    unique_ptr< ImageBuf > outBuf( new ImageBuf );

    // BUG with workaround
    // 'resample' is faster but creates black outlines when scaling up, so only
    // use it for scaling down.
    if( (spec.width  < inBuf->spec().width )
     && (spec.height < inBuf->spec().height) ){
        ImageBufAlgo::resample( *outBuf, *inBuf, false, roi );
    }
    else {
        ImageBufAlgo::resize( *outBuf, *inBuf, "", false, roi );
    }

    return outBuf;
}

//REMOVE
OIIO::ImageBuf
scale( const OIIO::ImageBuf& sourceBuf, const OIIO::ImageSpec& destSpec ) {

    const OIIO::ImageSpec& srcSpec = sourceBuf.spec();
    if( !sourceBuf.initialized() ) {
        SPDLOG_CRITICAL("srcbuf is not initialised" );
        return {};
    }

    // do nothing if the original is the correct size.
    if(srcSpec.width == destSpec.width && srcSpec.height == destSpec.height ){
        return {};
    }

    // Otherwise scale
    OIIO::ROI roi(0, destSpec.width, 0, destSpec.height, 0, 1, 0, srcSpec.nchannels );

    // resample is faster but creates black outlines when scaling up, so only
    // use it for scaling down.
    if(destSpec.width < srcSpec.width && destSpec.height < srcSpec.height ){
        return OIIO::ImageBufAlgo::resample( sourceBuf, false, roi );
    }
    else {
        return OIIO::ImageBufAlgo::resize( sourceBuf, "", false, roi );
    }
}