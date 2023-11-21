//
// Created by nicho on 17/11/2023.
//

#ifndef OIIO_H
#define OIIO_H
#include <fstream>
#include <filesystem>
#include <utility>

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>

#include "smfiobase.h"

class HeightIO final : public smflib::SMFIOBase {
    using Path = std::filesystem::path;

    Path _smfPath;
    OIIO::ImageSpec _imageSpec;
    OIIO::ImageBuf _imageBuffer;
public:
    explicit HeightIO();

    void setSMFPath( Path path ) { _smfPath = std::move(path); }
    void read() override;
    size_t write() override;

    void setWidth( int width );
    void setHeight( int height );

    void setImageBuf( OIIO::ImageBuf imageBuffer ) { _imageBuffer = std::move(imageBuffer); }
    OIIO::ImageBuf getImageBuf() { return _imageBuffer; }
};

#endif //OIIO_H
