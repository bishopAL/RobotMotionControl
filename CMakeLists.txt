cmake_minimum_required(VERSION 2.8.9)

PROJECT(Dym)

set( CMAKE_BUILD_TYPE "Debug" )
set( CMAKE_CXX_FLAGS_DEBUG "-O0 -Wall -g -ggdb" )
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

include_directories(${PROJECT_SOURCE_DIR}/include )
include_directories(/usr/local/include/)
include_directories(/usr/include/ )


AUX_SOURCE_DIRECTORY(./src DIR_SRCS)

ADD_EXECUTABLE(test ${DIR_SRCS})
target_link_libraries(test pthread dxl_sbc_c rt m CppLinuxSerial)
