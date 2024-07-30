cmake_minimum_required(VERSION 3.16)
project(Jde.App.Server)
set( CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../Public/build )
include(common)
#######################################################
link_directories( ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} )
link_directories( ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY} )
link_directories( ${Protobuf_DIR}/../.. )
find_package(ZLIB)
#add_subdirectory( source )
add_executable( ${CMAKE_PROJECT_NAME} )
file( GLOB src_files source/main.cpp )
file( GLOB hdr_files source/**/*.h )
#set(CMAKE_INCLUDE_CURRENT_DIR ON)
target_sources("${CMAKE_PROJECT_NAME}"
    PRIVATE
        "${src_files}"
        "${hdr_files}"
)

#######################################################
include_directories( "../Public" )

target_link_libraries( ${CMAKE_PROJECT_NAME} ZLIB::ZLIB )
target_link_libraries( ${CMAKE_PROJECT_NAME} debug libfmtd.a optimized libfmt.a )
target_link_libraries( ${CMAKE_PROJECT_NAME} Threads::Threads )
target_link_libraries( ${CMAKE_PROJECT_NAME} Jde Jde.App.Shared Jde.Crypto Jde.Web.Server Jde.Web.Client libJde.App.ServerLib.a )
target_link_libraries( ${CMAKE_PROJECT_NAME} dl )
target_link_libraries( ${CMAKE_PROJECT_NAME} lzma )
target_link_libraries( ${CMAKE_PROJECT_NAME} crypto )
target_link_libraries( ${CMAKE_PROJECT_NAME} ssl )
target_link_libraries( ${CMAKE_PROJECT_NAME} protobuf::libprotobuf )
target_link_libraries( ${CMAKE_PROJECT_NAME} stdc++ )
#######################################################
