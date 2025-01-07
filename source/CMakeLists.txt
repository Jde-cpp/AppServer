cmake_minimum_required(VERSION 3.20)
set( CMAKE_MODULE_PATH $ENV{JDE_DIR}/Public/build )
include(common)
findBoost()
if( ${VCPKG} )
	SET( CMAKE_TOOLCHAIN_FILE $ENV{REPO_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake  CACHE STRING "Vcpkg toolchain file" )
endif()
project(Jde.App.ServerLib)
include( compiler )
#######################################################
if( MSVC )
	link_directories( $ENV{JDE_DIR}/Public/src/web/client/.build/.bin )
endif()
#######################################################
add_library( ${CMAKE_PROJECT_NAME} )
file(GLOB sources *.cpp **/*.cpp )
get_filename_component( main_cpp ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp ABSOLUTE )
list( REMOVE_ITEM sources ${main_cpp} )
file( GLOB headers **/*.h )
target_sources( ${CMAKE_PROJECT_NAME} PRIVATE ${sources} ${headers} )
include_directories( $ENV{JDE_DIR}/Public/include/jde/web/client/proto )
#######################################################
if( MSVC )
	find_package( OpenSsl REQUIRED )
	find_package( protobuf REQUIRED )
	include_directories( ${OPENSSL_INCLUDE_DIR} )
#	include_directories( ${installDir}/include )
endif()
include_directories( "../../Public" )
#######################################################
target_precompile_headers( ${CMAKE_PROJECT_NAME} PRIVATE
	<jde/framework.h>
	<jde/framework/io/proto.h>
	"externals.h"
	"usings.h"
)
add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E remove "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Jde.App.Server" COMMENT "Remove exe" )