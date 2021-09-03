cmake_minimum_required(VERSION 3.16)
project(Jde.AppServer)
set( CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../../Framework/cmake )
include(SetUpConfigurations)
#######################################################
link_directories( ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} )

add_executable( Jde.AppServer main.cpp Listener.cpp Sender.cpp Cache.cpp LogData.cpp LogClient.cpp
			WebServer.cpp WebSession.cpp WebSocket.cpp
			types/proto/FromClient.pb.cc types/proto/FromServer.pb.cc )
#######################################################
include_directories( "../../Public" )
#target_link_libraries(Jde.AppServer)
#https://stackoverflow.com/questions/49482691/why-use-add-librarytgt-imported-versus-target-link-libraries-l-so-a
target_link_libraries(Jde.AppServer Threads::Threads)
target_link_libraries(Jde.AppServer Jde)
target_link_libraries(Jde.AppServer dl)
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
		"TypeDefs.h"
		<jde/Log.h>
		"../../Framework/source/log/server/proto/messages.pb.h"
)
