cmake_minimum_required(VERSION 2.6)

if (CMAKE_COMPILER_IS_GNUCXX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")  # Or -std=c++11
endif (CMAKE_COMPILER_IS_GNUCXX)

set(CLU_RUNTIME_SOURCES
    clu_runtime.cpp )

include_directories(
   ${OPENCL_DIST_DIR}/include)

add_library(clu_runtime ${CLU_RUNTIME_SOURCES})
