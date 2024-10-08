cmake_minimum_required(VERSION 3.20)
set( CMAKE_MODULE_PATH $ENV{JDE_DIR}/Public/build )
include(common)
findBoost()
if( ${VCPKG} )
	SET( CMAKE_TOOLCHAIN_FILE $ENV{REPO_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake  CACHE STRING "Vcpkg toolchain file" )
endif()
project( Jde.App.Server.Tests VERSION ${version} )
include(compiler)
if( NOT ${VCPKG} )
	include(tests)
endif()
if( MSVC )
	find_package( OpenSsl REQUIRED )
	link_directories( $ENV{JDE_DIR}/Public/src/app/shared/.build/.bin )
	link_directories( $ENV{JDE_DIR}/Framework/source/.build/.bin )
	link_directories( $ENV{JDE_DIR}/Public/src/web/server/.build/.bin )
	link_directories( $ENV{JDE_DIR}/Public/src/crypto/.build/.bin )
else()
	link_directories( ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} )
	link_directories( ${fmt_DIR}/../.. )
	link_directories( ${Protobuf_DIR}/../.. )
endif()

add_executable( ${CMAKE_PROJECT_NAME} main.cpp ThreadingTest.cpp DBTests.cpp um/UserTests.cpp um/GroupTests.cpp )

find_package( GTest REQUIRED )
target_link_libraries( ${CMAKE_PROJECT_NAME}  GTest::gtest_main )
if( MSVC )
	set_target_properties( ${CMAKE_PROJECT_NAME} PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "-c" )
	find_package( protobuf REQUIRED )
	include_directories( ${OPENSSL_INCLUDE_DIR} )
	target_link_libraries(${CMAKE_PROJECT_NAME} OpenSSL::SSL)
	target_link_libraries( ${CMAKE_PROJECT_NAME} fmt::fmt )
else()
	target_link_libraries( ${CMAKE_PROJECT_NAME} Jde)
	target_link_libraries( ${CMAKE_PROJECT_NAME} gtest_main)
	target_link_libraries( ${CMAKE_PROJECT_NAME} debug libfmtd.a optimized libfmt.a)
	target_link_libraries( ${CMAKE_PROJECT_NAME} protobuf::libprotobuf libutf8_range.a libutf8_validity.a )
	target_link_libraries( ${CMAKE_PROJECT_NAME} absl::cord absl::log_internal_check_op absl::log_internal_conditions absl::log_internal_message absl::raw_hash_set absl::status absl::statusor ) #absl::strings
	target_link_libraries( ${CMAKE_PROJECT_NAME} crypto )
	target_link_libraries( ${CMAKE_PROJECT_NAME} ssl )
endif()

target_precompile_headers( ${CMAKE_PROJECT_NAME} PRIVATE
	<jde/TypeDefs.h>
	"externals.h"
	<jde/log/Log.h>
	<jde/Exception.h>
)

if( MSVC )
	set( subdir .build/.bin/$(ConfigurationName) )
	set( bin ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(ConfigurationName) )

	add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
		$ENV{JDE_DIR}/Framework/source/${subdir}/Jde.dll
	 	$ENV{JDE_DIR}/Framework/source/${subdir}/Jde.pdb
		${bin} )

#add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $ENV{JDE_DIR}/Framework/source/${subdir}/Jde.dll ${bin}/Jde.dll )
#add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $ENV{JDE_DIR}/Framework/source/${subdir}/Jde.pdb ${bin}/Jde.pdb )
	add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $ENV{JDE_DIR}/Odbc/source/${subdir}/Jde.DB.Odbc.dll ${bin}/Jde.DB.Odbc.dll )
	add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $ENV{JDE_DIR}/Odbc/source/${subdir}/Jde.DB.Odbc.pdb ${bin}/Jde.DB.Odbc.pdb )
	add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $ENV{JDE_DIR}/Public/src/crypto/${subdir}/Jde.Crypto.dll ${bin}/Jde.Crypto.dll )
	add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $ENV{JDE_DIR}/Public/src/crypto/${subdir}/Jde.Crypto.pdb ${bin}/Jde.Crypto.pdb )
	add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $ENV{JDE_DIR}/Public/src/web/client/${subdir}/Jde.Web.Client.dll ${bin}/Jde.Web.Client.dll )
	add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $ENV{JDE_DIR}/Public/src/web/client/${subdir}/Jde.Web.Client.pdb ${bin}/Jde.Web.Client.pdb )
	add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $ENV{JDE_DIR}/Public/src/web/server/${subdir}/Jde.Web.Server.dll ${bin}/Jde.Web.Server.dll )
	add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $ENV{JDE_DIR}/Public/src/web/server/${subdir}/Jde.Web.Server.pdb ${bin}/Jde.Web.Server.pdb )
	add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $ENV{JDE_DIR}/Public/src/app/shared/${subdir}/Jde.App.Shared.dll ${bin}/Jde.App.Shared.dll )
	add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $ENV{JDE_DIR}/Public/src/app/shared/${subdir}/Jde.App.Shared.pdb ${bin}/Jde.App.Shared.pdb )
	add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $ENV{JDE_DIR}/AppServer/config/meta.json $ENV{JDE_DIR}/AppServer/tests/config/meta.json )
endif()