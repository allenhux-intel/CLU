cmake_minimum_required(VERSION 2.6)

set(CLU_LIB_DIR ${CLU_SOURCE_DIR}/lib CACHE PATH "CLU Library dir")

set(GL_PARTICLES_SOURCES
    ToneMapping.cpp 
	)

set(GL_PARTICLES_HEADERS
	ToneMapping.cl
	${CLU_SOURCE_DIR}/samples/ToneMapping/ToneMapping.cl.h
	chdrdata.h
	)	

add_custom_command(
	OUTPUT ToneMapping.cl.h
	COMMAND clu_generator ToneMapping.cl
	DEPENDS ToneMapping.cl
	COMMENT "Generating through CLU ..."
	WORKING_DIRECTORY "${CLU_SOURCE_DIR}/samples/ToneMapping/")

include_directories(
   ${OPENCL_DIST_DIR}/include
   ${CLU_SOURCE_DIR}/clu_runtime
   ${GLEW_DIST_DIR}/include
   ${CLU_LIB_DIR})

if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
  link_directories( ${OPENCL_DIST_DIR}/lib/x86_64 )
else( CMAKE_SIZEOF_VOID_P EQUAL 8 )
  link_directories( ${OPENCL_DIST_DIR}/lib/x86 )
endif( CMAKE_SIZEOF_VOID_P EQUAL 8 )

add_executable(ToneMapping ${GL_PARTICLES_SOURCES} ${GL_PARTICLES_HEADERS})
add_dependencies(ToneMapping 
	clu_generator
	clu_runtime)
target_link_libraries( ToneMapping OpenCL.lib clu_runtime ) 