cmake_minimum_required(VERSION 2.6)

set(CPPVECTORADD_SOURCES
    CPPVectorAdd.cpp )

set(CPPVECTORADD_HEADERS
	CPPVectorAddKernel.cl
	${CMAKE_CURRENT_BINARY_DIR}/CPPVectorAddKernel.cl.h)

if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")  # Or -std=c++11
endif (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

add_custom_command(
	OUTPUT CPPVectorAddKernel.cl.h
	COMMAND clu_generator -o CPPVectorAddKernel.cl.h -cpp ${CMAKE_CURRENT_SOURCE_DIR}/CPPVectorAddKernel.cl
	DEPENDS CPPVectorAddKernel.cl
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

add_executable(CPPVectorAdd ${CPPVECTORADD_SOURCES} ${CPPVECTORADD_HEADERS})
add_dependencies(CPPVectorAdd 
	clu_generator
	clu_runtime)
target_link_libraries( CPPVectorAdd OpenCL clu_runtime ) 
