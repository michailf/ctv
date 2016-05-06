cmake_minimum_required(VERSION 3.0)
project(ctv C)

#set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
#set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

include(CheckIncludeFile)
CHECK_INCLUDE_FILE(/opt/vc/include/bcm_host.h BCMHOST)

if(BCMHOST)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DRASPBERRY")
endif()

find_library(curl_LIBRARY NAMES curl)
find_package(OpenSSL REQUIRED)
find_package(LibXml2 REQUIRED)
find_library(ncurses_LIBRARY NAMES ncursesw)
find_library(json_LIBRARY NAMES json-c)

include(../../w/common/macros.cmake)
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(../../w/)
include_directories(${CMAKE_BINARY_DIR})
add_subdirectory(../../w/common bin)

gen_version_c()

list(APPEND SOURCES main.c api.c api.h version.c version.h util.c util.h)
list(APPEND LIBS  ${ncurses_LIBRARY} ${json_LIBRARY} ${curl_LIBRARY} svc)
if(BCMHOST)
	list(APPEND SOURCES joystick.c)
endif()

add_executable(ctv ${SOURCES})
add_dependencies(ctv mkversion mkresource)
target_link_libraries(ctv ${LIBS})
