add_library( smf_tools STATIC
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
        PUBLIC
            OpenImageIO::OpenImageIO
#            TIFF::tiff
#            png_static
            libjpeg-turbo::jpeg-static
            bcrypt
            spdlog::spdlog
            nlohmann_json::nlohmann_json
)

add_executable( smf_cc smf_cc.cpp )
target_include_directories( smf_cc PRIVATE ${CMAKE_SOURCE_DIR}/ext/optionsparser)
target_link_libraries( smf_cc smf_tools -static )
target_link_options( smf_cc PUBLIC "LINKER:--allow-multiple-definition")

add_executable( smf_decc smf_decc.cpp )
target_include_directories( smf_decc PRIVATE ${CMAKE_SOURCE_DIR}/ext/optionsparser)
target_link_libraries( smf_decc smf_tools -static )
target_link_options( smf_decc PUBLIC "LINKER:--allow-multiple-definition")

add_executable( smt_convert smt_convert.cpp )
target_include_directories( smt_convert PRIVATE ${CMAKE_SOURCE_DIR}/ext/optionsparser)
target_link_libraries( smt_convert smf_tools -static )
target_link_options( smt_convert PUBLIC "LINKER:--allow-multiple-definition")

add_executable( smt_info smt_info.cpp )
target_include_directories( smt_info PRIVATE ${CMAKE_SOURCE_DIR}/ext/optionsparser)
target_link_libraries( smt_info smf_tools -static )
target_link_options( smt_info PUBLIC "LINKER:--allow-multiple-definition")

add_executable( smf_info smf_info.cpp )
target_include_directories( smf_info PRIVATE ${CMAKE_SOURCE_DIR}/ext/optionsparser)
target_link_libraries( smf_info smf_tools -static )
target_link_options( smf_info PUBLIC "LINKER:--allow-multiple-definition")

add_executable( smt_repair smt_repair.cpp )
target_include_directories( smt_repair PRIVATE ${CMAKE_SOURCE_DIR}/ext/optionsparser)
target_link_libraries( smt_repair smf_tools -static )
target_link_options( smt_repair PUBLIC "LINKER:--allow-multiple-definition")

add_executable( mapconv mapconv.cpp )
target_include_directories( mapconv PRIVATE ${CMAKE_SOURCE_DIR}/ext/optionsparser)
target_link_libraries( mapconv smf_tools -static )
target_link_options( mapconv PUBLIC "LINKER:--allow-multiple-definition")

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
