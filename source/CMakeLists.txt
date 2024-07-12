cmake_minimum_required(VERSION 3.16)
project(Jde.AppServer)
set( CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../../Public/build )
include(common)
#######################################################
link_directories( ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} )
link_directories( ${Protobuf_DIR}/../.. )
find_package(ZLIB)
get_cmake_property(_variableNames VARIABLES)
add_executable( ${CMAKE_PROJECT_NAME} )
file(GLOB src_files *.cpp **/*.cpp )
file(GLOB hdr_files   "**/*.h")
#file(GLOB proto_files "**/*.proto")
set(CMAKE_INCLUDE_CURRENT_DIR ON)
target_sources("${CMAKE_PROJECT_NAME}"
    PRIVATE
        "${src_files}"
        "${hdr_files}"
#       "${proto_files}"
)

#######################################################
include_directories( "../../Public" )

target_link_libraries( ${CMAKE_PROJECT_NAME} ZLIB::ZLIB )
target_link_libraries( ${CMAKE_PROJECT_NAME} debug libfmtd.a optimized libfmt.a )
target_link_libraries( ${CMAKE_PROJECT_NAME} Threads::Threads )
target_link_libraries( ${CMAKE_PROJECT_NAME} Jde )
target_link_libraries( ${CMAKE_PROJECT_NAME} Jde.App.Client )
target_link_libraries( ${CMAKE_PROJECT_NAME} Jde.Crypto )
target_link_libraries( ${CMAKE_PROJECT_NAME} Jde.Ssl )
target_link_libraries( ${CMAKE_PROJECT_NAME} Jde.Web )
target_link_libraries( ${CMAKE_PROJECT_NAME} Jde.Http )
target_link_libraries( ${CMAKE_PROJECT_NAME} Jde.XZ )
target_link_libraries( ${CMAKE_PROJECT_NAME} dl )
target_link_libraries( ${CMAKE_PROJECT_NAME} lzma )
target_link_libraries( ${CMAKE_PROJECT_NAME} crypto )
target_link_libraries( ${CMAKE_PROJECT_NAME} ssl )
target_link_libraries( ${CMAKE_PROJECT_NAME} protobuf::libprotobuf )
target_link_libraries( ${CMAKE_PROJECT_NAME} stdc++ )
#######################################################
target_precompile_headers(${CMAKE_PROJECT_NAME}
	PRIVATE
		<filesystem>
		<forward_list>
		<set>
		<shared_mutex>
		<unordered_map>
		<boost/beast.hpp>
		<boost/asio.hpp>
		<boost/asio/ssl/error.hpp>
		<boost/exception/diagnostic_information.hpp>
		<boost/unordered/concurrent_flat_map.hpp>
		<boost/unordered/concurrent_flat_set.hpp>
		<jde/Exports.h>
		<jde/web/exports.h>
		"usings.h"
		"../../Public/src/appClient/.obj/${CMAKE_BUILD_TYPE}/App.FromServer.pb.h"
		"../../Public/src/appClient/.obj/${CMAKE_BUILD_TYPE}/App.FromClient.pb.h"
		"../../Public/src/appClient/.obj/${CMAKE_BUILD_TYPE}/Common.pb.h"
		<jde/log/Log.h>
		<jde/Str.h>
		"../../Framework/source/collections/UnorderedMap.h"
		"../../Framework/source/db/Database.h"
		"../../Framework/source/io/ProtoUtilities.h"
#		"../../Framework/source/io/ServerSink.h"
#		"../../Framework/source/io/Socket.h"
		"../../Framework/source/threading/Mutex.h"
		"../../Ssl/source/Ssl.h"
)
