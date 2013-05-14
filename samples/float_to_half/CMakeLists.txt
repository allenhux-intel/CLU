cmake_minimum_required(VERSION 2.6)

set(FLOAT_TO_HALF_SOURCES
    float_to_half.cpp )

include_directories(
   ${OPENCL_DIST_DIR}/include
   ${CLU_SOURCE_DIR}/clu_runtime)

if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
  link_directories( ${OPENCL_DIST_DIR}/lib/x86_64 )
else( CMAKE_SIZEOF_VOID_P EQUAL 8 )
  link_directories( ${OPENCL_DIST_DIR}/lib/x86 )
endif( CMAKE_SIZEOF_VOID_P EQUAL 8 )

add_executable(float_to_half ${FLOAT_TO_HALF_SOURCES} ${FLOAT_TO_HALF_HEADERS})
add_dependencies(float_to_half
	clu_generator
	clu_runtime)
target_link_libraries( float_to_half OpenCL.lib clu_runtime ) 