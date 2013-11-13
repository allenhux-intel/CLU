cmake_minimum_required(VERSION 2.6)

set(SIMPLE_SOURCES
    simple.c )

set(SIMPLE_HEADERS
	simple.cl
	${CMAKE_CURRENT_BINARY_DIR}/simple.cl.h)

add_custom_command(
	OUTPUT simple.cl.h
	COMMAND clu_generator -o simple.cl.h ${CMAKE_CURRENT_SOURCE_DIR}/simple.cl
	DEPENDS simple.cl
	COMMENT "Generating through CLU ...")

include_directories(
   ${OPENCL_DIST_DIR}/include
   ${CLU_SOURCE_DIR}/clu_runtime
   ${CMAKE_CURRENT_BINARY_DIR})

if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
  link_directories( ${OPENCL_DIST_DIR}/lib/x86_64 )
else( CMAKE_SIZEOF_VOID_P EQUAL 8 )
  link_directories( ${OPENCL_DIST_DIR}/lib/x86 )
endif( CMAKE_SIZEOF_VOID_P EQUAL 8 )

add_executable(simple ${SIMPLE_SOURCES} ${SIMPLE_HEADERS})
add_dependencies(simple
	clu_generator
	clu_runtime)
target_link_libraries( simple OpenCL clu_runtime )
