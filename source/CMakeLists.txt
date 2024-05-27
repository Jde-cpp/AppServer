cmake_minimum_required(VERSION 3.16)
project(Jde.AppServer)
set( CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../../Public/build )
include(common)
#######################################################
link_directories( ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} )
link_directories( ${Protobuf_DIR}/../.. )
find_package(ZLIB)
get_cmake_property(_variableNames VARIABLES)

add_executable( ${CMAKE_PROJECT_NAME} main.cpp Cache.cpp GoogleLogin.cpp Listener.cpp LogData.cpp LogClient.cpp
			Rest.cpp WebServer.cpp WebSession.cpp
			types/proto/AppFromClient.pb.cc types/proto/AppFromServer.pb.cc )
#######################################################
include_directories( "../../Public" )

target_link_libraries( ${CMAKE_PROJECT_NAME} ZLIB::ZLIB )
target_link_libraries( ${CMAKE_PROJECT_NAME} debug libfmtd.a optimized libfmt.a )
target_link_libraries( ${CMAKE_PROJECT_NAME} Threads::Threads )
target_link_libraries( ${CMAKE_PROJECT_NAME} Jde )
target_link_libraries( ${CMAKE_PROJECT_NAME} Jde.Crypto )
target_link_libraries( ${CMAKE_PROJECT_NAME} Jde.Ssl )
target_link_libraries( ${CMAKE_PROJECT_NAME} Jde.Web )
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
		<jde/Exports.h>
		"../../Public/src/web/Exports.h"
		"TypeDefs.h"
		"types/proto/AppFromServer.pb.h"
		"types/proto/AppFromClient.pb.h"
		<jde/Log.h>
		<jde/Str.h>
		"../../Framework/source/collections/UnorderedMap.h"
		"../../Framework/source/db/Database.h"
		"../../Framework/source/io/ProtoUtilities.h"
		"../../Framework/source/io/ServerSink.h"
		"../../Framework/source/io/Socket.h"
		"../../Framework/source/threading/Mutex.h"
		"../../Ssl/source/Ssl.h"
)
