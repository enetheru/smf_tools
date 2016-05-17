#include <iostream>
#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>

int main()
{
    OIIO_NAMESPACE_USING
    // Create input buffer with one channel.
    ImageSpec inputSpec( 32, 32, 4, TypeDesc::FLOAT );

    ImageBuf inputBuf( inputSpec );
    switch( inputBuf.storage() ){
        case ImageBuf::IBStorage::UNINITIALIZED:
            std::cout << " unitialized\n";
            exit(0);
            break;
        case ImageBuf::IBStorage::LOCALBUFFER:
            std::cout << " localbuffer\n";
            break;
        case ImageBuf::IBStorage::APPBUFFER:
            std::cout << " appbuffer\n";
            break;
        case ImageBuf::IBStorage::IMAGECACHE:
            std::cout << " imagecache\n";
            break;
    }


    // create map and fill variables to use in the channels function
    int map[] = { 0, 1, 2, 3 };
    float fill[] = { 0, 0, 0, 1.0 };

    //create output buffer
    ImageBuf outputBuf;

    // run function
    ImageBufAlgo::channels( outputBuf, inputBuf, 1, map, fill );
}
