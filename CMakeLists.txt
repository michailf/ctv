cmake_minimum_required(VERSION 3.0)
project(ctv C)

find_library(curl_LIBRARY NAMES curl)
find_package(OpenSSL REQUIRED)
find_package(LibXml2 REQUIRED)
find_library(ncurses_LIBRARY NAMES ncursesw)
find_library(json_LIBRARY NAMES json-c)

include(../../../w/common/macros.cmake)
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(../../../w/)
include_directories(${CMAKE_BINARY_DIR})
add_subdirectory(../../../w/common bin)

gen_version_c()

list(APPEND SOURCES
	main.c
	provider.c provider.h
	etvnet.c etvnet.h
	smithsonian.c smithsonian.h
	version.c version.h
	util.c util.h
	joystick.c joystick.h
)
list(APPEND LIBS ${ncurses_LIBRARY} ${json_LIBRARY} ${curl_LIBRARY} svc)

add_executable(ctv ${SOURCES})
add_dependencies(ctv mkversion mkresource)
target_link_libraries(ctv ${LIBS})

add_executable(joystick-test joystick.c joystick-test.c)
target_link_libraries(joystick-test ${ncurses_LIBRARY} svc)

add_executable(smith-parse smith-parse.c util.c smithsonian.c)
target_link_libraries(smith-parse ${LIBS})
