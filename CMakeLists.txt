cmake_minimum_required(VERSION 2.6)
project(CLU)

set(OPENCL_DIST_DIR $ENV{AMDAPPSDKROOT} CACHE PATH "OpenCL source dir")

add_subdirectory(samples)
add_subdirectory(clu_generator)
add_subdirectory(clu_runtime)
add_subdirectory(simple)