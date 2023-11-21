
add_library(smflib STATIC
        smf.cpp smf.h
#        smt.cpp smt.h
#        tilemap.cpp tilemap.h
#        tilecache.cpp tilecache.h
#        tiledimage.cpp tiledimage.h
        util.cpp util.h
        filemap.cpp filemap.h
        SMFIOBase.h
#        oiio.cpp oiio.h
        smf.h
        nullio.cpp
        nullio.h
        basicio.cpp
        basicio.h
)
target_sources(smflib INTERFACE
        smf.h
        smt.h
)

target_include_directories(smflib
        PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        PUBLIC
        ${CMAKE_SOURCE_DIR}/ext
)
target_link_libraries(smflib
        PRIVATE
#        OpenEXR::OpenEXR
#        Boost::filesystem
#        Boost::thread
#        libsquish::Squish
#        ZLIB::ZLIB
        PUBLIC
#        OpenImageIO::OpenImageIO
        #            TIFF::tiff
        #            png_static
#        libjpeg-turbo::jpeg-static
#        bcrypt
        spdlog::spdlog
        nlohmann_json::nlohmann_json
)