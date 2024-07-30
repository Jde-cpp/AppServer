cmake_minimum_required(VERSION 3.16)
project(Jde.App.ServerLib)
set( CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../../Public/build )
include(common)
#######################################################
#link_directories( ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} )
#link_directories( ${Protobuf_DIR}/../.. )
#find_package(ZLIB)
get_cmake_property(_variableNames VARIABLES)
add_library( ${CMAKE_PROJECT_NAME} )
file(GLOB src_files *.cpp **/*.cpp )
get_filename_component(main_cpp ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp ABSOLUTE)
list(REMOVE_ITEM src_files "${main_cpp}")

file(GLOB hdr_files   "**/*.h")
set(CMAKE_INCLUDE_CURRENT_DIR ON)
target_sources("${CMAKE_PROJECT_NAME}"
    PRIVATE
        "${src_files}"
        "${hdr_files}"
)

#######################################################
include_directories( "../../Public" )

#target_link_libraries( ${CMAKE_PROJECT_NAME} ZLIB::ZLIB )
#target_link_libraries( ${CMAKE_PROJECT_NAME} debug libfmtd.a optimized libfmt.a )
#target_link_libraries( ${CMAKE_PROJECT_NAME} Threads::Threads )
#target_link_libraries( ${CMAKE_PROJECT_NAME} Jde Jde.App.Client Jde.Crypto Jde.Web.Server Jde.Web.Client )
#target_link_libraries( ${CMAKE_PROJECT_NAME} dl )
#target_link_libraries( ${CMAKE_PROJECT_NAME} lzma )
#target_link_libraries( ${CMAKE_PROJECT_NAME} crypto )
#target_link_libraries( ${CMAKE_PROJECT_NAME} ssl )
#target_link_libraries( ${CMAKE_PROJECT_NAME} protobuf::libprotobuf )
#target_link_libraries( ${CMAKE_PROJECT_NAME} stdc++ )
#######################################################
target_precompile_headers(${CMAKE_PROJECT_NAME}
	PRIVATE
		<filesystem>
		<boost/beast.hpp>
		<boost/asio.hpp>
		<boost/asio/ssl/error.hpp>
		<boost/beast/ssl/ssl_stream.hpp>
		<boost/exception/diagnostic_information.hpp>
		<boost/unordered/concurrent_flat_map.hpp>
		<boost/unordered/concurrent_flat_set.hpp>
		"usings.h"
		<jde/log/Log.h>
		<jde/Exception.h>
		<jde/Str.h>
		<jde/web/server/exports.h>
		<jde/app/shared/proto/App.FromServer.pb.h>
		<jde/app/shared/proto/App.FromClient.pb.h>
		<jde/app/shared/proto/Common.pb.h>
		"../../Framework/source/db/Database.h"
		"../../Framework/source/io/ProtoUtilities.h"
)
