cmake_minimum_required(VERSION 3.16)
project(Jde.AppServer)
set( CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../../Framework/cmake )
include(SetUpConfigurations)
#######################################################
link_directories( ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} )
find_package(ZLIB)

add_executable( Jde.AppServer main.cpp Cache.cpp GoogleLogin.cpp Listener.cpp LogData.cpp LogClient.cpp
			Rest.cpp WebServer.cpp WebSession.cpp WebSocketAsync.cpp
			types/proto/FromClient.pb.cc types/proto/FromServer.pb.cc )
#######################################################
include_directories( "../../Public" )
#target_link_libraries(Jde.AppServer)
target_link_libraries( Jde.AppServer ZLIB::ZLIB )
target_link_libraries(Jde.AppServer Threads::Threads)
target_link_libraries(Jde.AppServer Jde)
target_link_libraries(Jde.AppServer Jde.Ssl)
target_link_libraries(Jde.AppServer Jde.XZ)
target_link_libraries(Jde.AppServer dl)
target_link_libraries( Jde.AppServer lzma )
target_link_libraries( Jde.AppServer crypto )
target_link_libraries( Jde.AppServer ssl )
target_link_libraries(Jde.AppServer protobuf)
target_link_libraries(Jde.AppServer stdc++)
#target_link_libraries(Jde.AppServer boost_system)
#target_link_libraries(Jde.AppServer boost_thread)
#target_link_libraries(Jde.AppServer mysqlcppconn8)
#target_link_libraries(Jde.AppServer Jde.MySql)
#######################################################
target_precompile_headers(Jde.AppServer
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
		"TypeDefs.h"
		<jde/Log.h>
		<jde/Str.h>
		<jde/log/types/proto/FromServer.pb.h>
		"../../Framework/source/db/Database.h"
		"../../Framework/source/log/server/proto/messages.pb.h"
		"../../Ssl/source/Ssl.h"
)
