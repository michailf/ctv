cmake_minimum_required(VERSION 3.2)
project(ctv C)

set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")


find_library(curl_LIBRARY NAMES curl)
find_package(OpenSSL REQUIRED)
find_package(LibXml2 REQUIRED)
find_library(json_LIBRARY NAMES json-c)

include(../../w/common/macros.cmake)
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(../../w/)
include_directories(${CMAKE_BINARY_DIR})
add_subdirectory(../../w/common bin)

gen_version_c()

add_executable(ctv main.c api.c version.c api.h)
add_dependencies(ctv mkversion mkresource)
target_link_libraries(ctv ncurses ${json_LIBRARY} ${curl_LIBRARY} svc)
