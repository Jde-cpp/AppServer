cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )

include( $ENV{JDE_DIR}/Public/build/functions.cmake )
project( Jde.App.Server VERSION ${JDE_VERSION} LANGUAGES CXX )

#link_directories( ${JSONNET_DIR}/lib )

add_subdirectory( $ENV{JDE_DIR}/Framework/source ../../libs/framework )
add_subdirectory( $ENV{JDE_DIR}/Public/libs/crypto/src ../../libs/crypto )
add_subdirectory( $ENV{JDE_DIR}/Public/libs/db/src ../../libs/db/lib )
if( CMAKE_SYSTEM_NAME STREQUAL "Linux" )
	add_subdirectory( $ENV{JDE_DIR}/Public/libs/db/drivers/mysql ../../libs/db/drivers/mysql )
endif()
add_subdirectory( $ENV{JDE_DIR}/Public/libs/ql ../../libs/ql )
add_subdirectory( $ENV{JDE_DIR}/Public/libs/access/src ../../libs/access/lib )
add_subdirectory( $ENV{JDE_DIR}/Public/libs/web/server ../../libs/web/server )
add_subdirectory( $ENV{JDE_DIR}/Public/libs/web/client ../../libs/web/client )
add_subdirectory( $ENV{JDE_DIR}/Public/libs/app/shared ../../libs/app/shared )

add_subdirectory( source lib )

# if( buildTests )
# 	add_subdirectory( tests )
# 	set( CMAKE_CTEST_ARGUMENTS -c -settings=$ENV{JDE_DIR}/AppServer/config/App.Server.jsonnet )
# 	add_test( NAME AppTests COMMAND $<TARGET_FILE:Jde.App.Tests> )
# endif()

include( $ENV{JDE_DIR}/Public/build/dependencies.cmake )
set( targetName Jde.App.Server )
project( ${targetName} )
add_executable( ${targetName} )
set( sources source/main.cpp )
if( WIN32 )
	list( APPEND sources source/AppServer.rc )
endif()
file( GLOB headers source/**/*.h )
target_sources( ${targetName} PRIVATE ${sources} ${headers} )

include_directories( $ENV{JDE_DIR}/Public/include/jde/web/client/proto )

target_link_libraries( ${targetName} OpenSSL::SSL )
target_link_libraries( ${targetName} Boost::json )
target_link_libraries( ${targetName} protobuf::libprotobuf )
#target_link_libraries( ${targetName} jsonnet )
target_link_libraries( ${targetName} Jde Jde.QL Jde.Web.Server Jde.App.ServerLib )

add_custom_command( TARGET ${targetName} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "$ENV{JDE_DIR}/Public/libs/db/config/common-meta.libsonnet" "$ENV{JDE_DIR}/AppServer/config/common-meta.libsonnet" COMMENT "mklink common-meta.libsonnet" )