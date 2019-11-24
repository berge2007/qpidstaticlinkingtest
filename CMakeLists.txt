cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

project(x CXX)

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)

list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_SOURCE_DIR}/external/cmake-modules")

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -v -std=c++17 -lpthread -static-libgcc -static-libstdc++ -fdiagnostics-color=auto -lstdc++fs -latomic -g")

add_subdirectory(src)
