add_library( smf_tools
    smf.cpp         smf.h
    smt.cpp         smt.h
    tilemap.cpp     tilemap.h
    tilecache.cpp   tilecache.h
    tiledimage.cpp  tiledimage.h
    util.cpp        util.h
    filemap.cpp     filemap.h
)

target_include_directories( smf_tools
        PRIVATE
            ${CMAKE_CURRENT_BINARY_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}
        PUBLIC
            ${CMAKE_SOURCE_DIR}/ext
)
target_link_libraries( smf_tools
        PRIVATE
            OpenEXR::OpenEXR
            Boost::filesystem
            Boost::thread
            libsquish::Squish
            ZLIB::ZLIB
            libjpeg-turbo::turbojpeg-static

        PUBLIC
            OpenImageIO::OpenImageIO
            TIFF::tiff
            png_static
            libjpeg-turbo::jpeg-static
            bcrypt
            spdlog::spdlog
)

add_executable( smf_cc smf_cc.cpp )
target_include_directories( smf_cc PRIVATE ${CMAKE_SOURCE_DIR}/ext/optionsparser)
target_link_libraries( smf_cc smf_tools -static )

add_executable( smf_decc smf_decc.cpp )
target_include_directories( smf_decc PRIVATE ${CMAKE_SOURCE_DIR}/ext/optionsparser)
target_link_libraries( smf_decc smf_tools -static )

add_executable( smt_convert smt_convert.cpp )
target_include_directories( smt_convert PRIVATE ${CMAKE_SOURCE_DIR}/ext/optionsparser)
target_link_libraries( smt_convert smf_tools -static )

add_executable( smt_info smt_info.cpp )
target_include_directories( smt_info PRIVATE ${CMAKE_SOURCE_DIR}/ext/optionsparser)
target_link_libraries( smt_info smf_tools -static )

add_executable( smf_info smf_info.cpp )
target_include_directories( smf_info PRIVATE ${CMAKE_SOURCE_DIR}/ext/optionsparser)
target_link_libraries( smf_info smf_tools -static )

add_executable( smt_repair smt_repair.cpp )
target_include_directories( smt_repair PRIVATE ${CMAKE_SOURCE_DIR}/ext/optionsparser)
target_link_libraries( smt_repair smf_tools -static )

add_executable( mapconv mapconv.cpp )
target_include_directories( mapconv PRIVATE ${CMAKE_SOURCE_DIR}/ext/optionsparser)
target_link_libraries( mapconv smf_tools -static )

INSTALL(PROGRAMS
    ${CMAKE_CURRENT_BINARY_DIR}/smf_cc
    ${CMAKE_CURRENT_BINARY_DIR}/smf_decc
    ${CMAKE_CURRENT_BINARY_DIR}/smf_info
    ${CMAKE_CURRENT_BINARY_DIR}/smt_convert
    ${CMAKE_CURRENT_BINARY_DIR}/smt_info
    ${CMAKE_CURRENT_BINARY_DIR}/smt_repair
    ${CMAKE_CURRENT_SOURCE_DIR}/makemap.sh
    ${CMAKE_CURRENT_SOURCE_DIR}/mapconv
           DESTINATION bin)
