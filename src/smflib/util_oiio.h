//
// Created by nicho on 20/11/2023.
//

#ifndef UTIL_OIIO_H
#define UTIL_OIIO_H

#include <OpenImageIO/imagebuf.h>

/// re-orders the channels an imageBuf according to the imageSpec
/*  Returns a copy of the sourceBuf with the number of channels in
 *  the ImageSpec spec.
 *  If there is no Alpha then an opaque one is created.
 */
OIIO::ImageBuf channels( const OIIO::ImageBuf &sourceBuf, const OIIO::ImageSpec& destSpec );

/// Scales an ImageBuf according to a given ImageSpec
OIIO::ImageBuf scale( const OIIO::ImageBuf &sourceBuf, const OIIO::ImageSpec &destSpec );

#endif //UTIL_OIIO_H
