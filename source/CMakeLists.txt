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
target_link_libraries(Jde.AppServer Threads::Threads)
#https://stackoverflow.com/questions/49482691/why-use-add-librarytgt-imported-versus-target-link-libraries-l-so-a
target_link_libraries(Jde.AppServer Jde)
target_link_libraries(Jde.AppServer dl)
target_link_libraries(Jde.AppServer protobuf)
#target_link_libraries(Jde.AppServer stdc++fs)
target_link_libraries(Jde.AppServer stdc++)
#######################################################
target_precompile_headers(Jde.AppServer
	PRIVATE
		<filesystem>
		<list>
		<nlohmann/json.hpp>
		<spdlog/spdlog.h>
		<spdlog/sinks/basic_file_sink.h>
		<spdlog/fmt/ostr.h>
		<boost/crc.hpp>
		<boost/system/error_code.hpp>
		<boost/asio.hpp>
		<boost/exception/diagnostic_information.hpp>
		<boost/asio/ts/io_context.hpp>
		<boost/beast/websocket.hpp>
		<boost/beast/core/buffers_to_string.hpp>
		"TypeDefs.h"
		"../../Framework/source/JdeAssert.h"
		"../../Framework/source/DateTime.h"
		"../../Framework/source/StringUtilities.h"
		"../../Framework/source/Diagnostics.h"
		"../../Framework/source/Settings.h"
		"../../Framework/source/log/Logging.h"
		"../../Framework/source/log/server/ServerSink.h"
		"../../Framework/source/db/Database.h"
		"../../Framework/source/db/Row.h"
		"../../Framework/source/threading/Thread.h"
		"../../Framework/source/collections/UnorderedMap.h"
		"../../Framework/source/collections/UnorderedSet.h"
		"../../Framework/source/io/sockets/ProtoServer.h"
		"../../Framework/source/collections/UnorderedMap.h"
		"../../Framework/source/collections/Queue.h"
		"../../Framework/source/db/DBQueue.h"
		"../../Framework/source/threading/InterruptibleThread.h"
		"../../Framework/source/application/Application.h"
		"../../Framework/source/io/sockets/ProtoClient.h"
		"../../Framework/source/io/File.h"
		"../../Framework/source/log/server/proto/messages.pb.h"
)
