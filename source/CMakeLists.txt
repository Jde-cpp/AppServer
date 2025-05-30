cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( $ENV{JDE_DIR}/Public/build/functions.cmake )
project( Jde.App.ServerLib )

include( $ENV{JDE_DIR}/Public/build/dependencies.cmake )

set( targetName Jde.App.ServerLib )
add_library( ${targetName} )
compileOptions( ${targetName} )
file(GLOB sources *.cpp **/*.cpp )
get_filename_component( main_cpp ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp ABSOLUTE )
list( REMOVE_ITEM sources ${main_cpp} )
file( GLOB headers **/*.h )
target_sources( ${targetName} PRIVATE ${sources} ${headers} )

include_directories( $ENV{JDE_DIR}/Public/include/jde/web/client/proto )
target_link_libraries( ${targetName} Jde.Access Jde.Web.Server Jde.App.Shared )
target_precompile_headers( ${targetName} PRIVATE
	<jde/framework.h>
	<jde/framework/io/proto.h>
	externals.h
	usings.h
)