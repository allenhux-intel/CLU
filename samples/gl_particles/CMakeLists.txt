cmake_minimum_required(VERSION 2.6)

set(GLEW_DIST_DIR ../lib/glew-1.7.0/ CACHE PATH "GLEW source dir")
set(CLU_LIB_DIR ${CLU_SOURCE_DIR}/samples/lib CACHE PATH "CLU Library dir")

find_package(OpenGL)
if (OPENGL_FOUND)
   MESSAGE("OpenGL Correctly Found")
   include_directories(${OPENGL_INCLUDE_DIR})
else (OPENGL_FOUND)
   MESSAGE("OpenGL environment missing")
endif (OPENGL_FOUND)

set(GL_PARTICLES_SOURCES
    gl_particles_main.cpp 
	RenderGL.cpp
	SimulateCL.cpp
	${GLEW_DIST_DIR}/src/glew.c
	)

set(GL_PARTICLES_HEADERS
	Simulate.cl
	${CLU_SOURCE_DIR}/samples/gl_particles/Simulate.cl.h
	SimulateCL.h
	RenderGL.h
	ParticleFS.gl
	ParticleVS.gl
	${CLU_LIB_DIR}/math/mathlib.h
	${CLU_LIB_DIR}/display/GLContext.h
	${CLU_LIB_DIR}/display/SampleWindow.h)	

add_custom_command(
	OUTPUT Simulate.cl.h
	COMMAND clu_generator Simulate.cl
	DEPENDS Simulate.cl
	COMMENT "Generating through CLU ..."
	WORKING_DIRECTORY "${CLU_SOURCE_DIR}/samples/gl_particles/")

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

add_executable(gl_particles WIN32 ${GL_PARTICLES_SOURCES} ${GL_PARTICLES_HEADERS})
add_dependencies(gl_particles 
	clu_generator
	clu_runtime)
target_link_libraries( gl_particles OpenCL.lib clu_runtime ${OPENGL_LIBRARIES} ) 
