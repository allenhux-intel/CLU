cmake_minimum_required(VERSION 2.6)

ADD_DEFINITIONS(
    -std=c++0x  # Or -std=c++11 
)

set(CLU_RUNTIME_SOURCES
    clu_runtime.cpp )

include_directories(
   ${OPENCL_DIST_DIR}/include)

add_library(clu_runtime ${CLU_RUNTIME_SOURCES})
