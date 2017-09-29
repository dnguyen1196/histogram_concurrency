cmake_minimum_required(VERSION 3.8)
project(histogram)


SET(GCC_COVERAGE_COMPILE_FLAGS "-pthread")
SET(GCC_COVERAGE_LINK_FLAGS    "-pthread")

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}" )
set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES histogram.cxx ee193_utils.cxx)
add_executable(histogram ${SOURCE_FILES})