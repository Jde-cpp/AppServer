cmake_minimum_required(VERSION 3.16)
project( Tests.App VERSION 1.0.0.0 )
set( CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../../Framework/cmake )
include(SetUpConfigurations)

configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

# Prevent overriding the parent project's compiler/linker settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This defines the gtest and gtest_main targets.
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
                 ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
                 EXCLUDE_FROM_ALL)


link_directories( ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} )
add_executable(Tests.App main.cpp ThreadingTest.cpp
	../source/Cache.cpp ../source/Listener.cpp ../source/LogClient.cpp ../source/LogData.cpp ../source/WebServer.cpp ../source/WebSession.cpp ../source/WebSocketAsync.cpp
	../source/types/proto/FromServer.pb.cc ../source/types/proto/FromClient.pb.cc )
include_directories( "$ENV{REPO_DIR}/tws-api/source/cppclient/client" )
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
#target_link_libraries(Tests.App c++abi)
target_link_libraries(Tests.App Jde)
#target_link_libraries(Tests.App Jde.XZ)
#target_link_libraries(Tests.App Jde.Tws)
#target_link_libraries(Tests.App Jde.Dts)
#target_link_libraries(Tests.App lzma)
#target_link_libraries(Tests.App Ib)
target_link_libraries(Tests.App protobuf)
target_link_libraries(Tests.App gtest_main)
target_link_libraries(Tests.App stdc++)
#target_link_libraries(Tests.App stdc++fs)


#target_include_directories(Tests.App PUBLIC ${REPO_DIR}/llvm-project/build/lib)
#add_test(NAME HistoricalDataCacheTest COMMAND MethodBarDoesAbc)
target_precompile_headers(Tests.App PRIVATE
	<boost/uuid/uuid.hpp>
	<boost/uuid/uuid_generators.hpp>
	<boost/uuid/uuid_io.hpp>
	<boost/crc.hpp>
	<boost/system/error_code.hpp>
	<spdlog/spdlog.h>
	<spdlog/sinks/basic_file_sink.h>
	<spdlog/fmt/ostr.h>
	<nlohmann/json.hpp>

	<jde/TypeDefs.h>
	<jde/Log.h>
	"../../Framework/source/DateTime.h"
	"../../Framework/source/log/server/proto/messages.pb.h"
	"../source/TypeDefs.h"
	"../source/types/proto/FromServer.pb.h"
	"../source/types/proto/FromClient.pb.h"
)