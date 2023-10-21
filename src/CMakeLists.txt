configure_file( smf_tools.h.in smf_tools.h )

add_library( smf_tools
    smf.cpp         smf.h
    smt.cpp         smt.h
    tilemap.cpp     tilemap.h
    tilecache.cpp   tilecache.h
    tiledimage.cpp  tiledimage.h
    util.cpp        util.h
)

target_include_directories( smf_tools
        PRIVATE
            ${CMAKE_CURRENT_BINARY_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_SOURCE_DIR}/ext/elog
            ${CMAKE_SOURCE_DIR}/ext/squish
)
target_link_libraries( smf_tools
        PRIVATE
            OpenEXR::OpenEXR
            Boost::filesystem
            Boost::thread
            squish
            ZLIB::ZLIB
            libjpeg-turbo::turbojpeg-static
        PUBLIC
            OpenImageIO::OpenImageIO
            TIFF::tiff
            libjpeg-turbo::jpeg-static
            bcrypt
            spdlog::spdlog
)

add_executable( smf_cc smf_cc.cpp )
target_include_directories( smf_cc
        PRIVATE
        ${CMAKE_SOURCE_DIR}/ext/optionsparser
)
target_link_libraries( smf_cc smf_tools -static )

add_executable( smf_decc smf_decc.cpp )
target_include_directories( smf_decc
        PRIVATE
            ${CMAKE_SOURCE_DIR}/ext/optionsparser
)
target_link_libraries( smf_decc smf_tools -static )

add_executable( smt_convert smt_convert.cpp )
target_include_directories( smt_convert
        PRIVATE
            ${CMAKE_SOURCE_DIR}/ext/optionsparser
)
target_link_libraries( smt_convert smf_tools -static )

add_executable( smt_info smt_info.cpp )
target_include_directories( smt_info
        PRIVATE
            ${CMAKE_SOURCE_DIR}/ext/optionsparser
)
target_link_libraries( smt_info smf_tools -static )

add_executable( smf_info smf_info.cpp )
target_include_directories( smf_info
        PRIVATE
        ${CMAKE_SOURCE_DIR}/ext/optionsparser
)
target_link_libraries( smf_info smf_tools -static )

add_executable( smt_repair smt_repair.cpp )
target_include_directories( smt_repair
        PRIVATE
            ${CMAKE_SOURCE_DIR}/ext/optionsparser
)
target_link_libraries( smt_repair smf_tools -static )

INSTALL(PROGRAMS
    ${CMAKE_CURRENT_BINARY_DIR}/smf_cc
    ${CMAKE_CURRENT_BINARY_DIR}/smf_decc
    ${CMAKE_CURRENT_BINARY_DIR}/smf_info
    ${CMAKE_CURRENT_BINARY_DIR}/smt_convert
    ${CMAKE_CURRENT_BINARY_DIR}/smt_info
    ${CMAKE_CURRENT_BINARY_DIR}/smt_repair
    ${CMAKE_CURRENT_SOURCE_DIR}/makemap.sh
           DESTINATION bin)
