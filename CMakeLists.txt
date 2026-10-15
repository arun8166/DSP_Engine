cmake_minimum_required(VERSION 3.10)
project(dspengine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

file(GLOB_RECURSE SOURCES src/*.cpp)
add_executable(dspengine ${SOURCES})
target_include_directories(dspengine PRIVATE src)
