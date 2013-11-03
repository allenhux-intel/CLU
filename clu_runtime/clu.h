/*
Copyright (c) 2012, Intel Corporation

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __CLU_H
#define __CLU_H

#ifdef APPLE
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#ifdef _MSC_VER
  #define INLINE __inline /* "inline" not supported for C files in VS2010 */
#else
  #define INLINE inline   /* use standard inline */
#endif

/******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
#define CLU_API_ENTRY 
#define CLU_API_CALL  __stdcall
#define CLU_CALLBACK  __stdcall
#else
#define CLU_API_ENTRY
#define CLU_API_CALL
#define CLU_CALLBACK
#endif

#define CLU_ND1(x)          cluNDRange1(x,0,0)
#define CLU_ND2(x,y)        cluNDRange2(x,y, 0,0, 0,0)
#define CLU_ND3(x,y,z)      cluNDRange3(x,y,z, 0,0,0, 0,0,0)

#define CLU_DEFAULT_Q       cluGetCommandQueue(CL_DEVICE_TYPE_DEFAULT,0)
#define CLU_GPU_Q           cluGetCommandQueue(CL_DEVICE_TYPE_GPU,0)
#define CLU_CPU_Q           cluGetCommandQueue(CL_DEVICE_TYPE_CPU,0)

#define CLU_CONTEXT         cluGetContext()

#define CLU_DEFAULT_PARAMS  cluGetDefaultParams()

/********************************************************************************************************/
/* CLU structures                                                                                       */
/********************************************************************************************************/

typedef struct
{
    cl_uint         dim; 
    size_t          global[3];
    size_t          local[3];
    size_t          offset[3];
} clu_nd_range;

typedef struct
{
    clu_nd_range     nd_range;
    cl_command_queue queue;                   /* may be NULL (uses default) */
    cl_uint          num_events_in_wait_list; /* may be NULL */
    cl_event*        event_wait_list;         /* may be NULL */
    cl_event*        out_event; /* may be NULL: application-provided return event */
} clu_enqueue_params;

typedef struct
{
    const char*                 vendor_name;           /* may be NULL */
    cl_context                  existing_context;      /* may be NULL */
    const char*                 compile_options;       /* may be NULL */
    cl_command_queue_properties default_queue_props;   /* may be NULL */
    cl_context_properties*      default_context_props; /* may be NULL */
    cl_device_type              preferred_device_type; /* may be NULL */
} clu_initialize_params;

/********************************************************************************************************/
/* Platform API                                                                                         */
/********************************************************************************************************/

CLU_API_ENTRY clu_nd_range CLU_API_CALL cluNDRange1(int global_dim_1, int local_dim_1, int offset_1);
CLU_API_ENTRY clu_nd_range CLU_API_CALL cluNDRange2(int global_dim_1, int global_dim_2, int local_dim_1, int local_dim_2, int offset_1, int offset_2);
CLU_API_ENTRY clu_nd_range CLU_API_CALL cluNDRange3(int global_dim_1, int global_dim_2, int global_dim_3, int local_dim_1, int local_dim_2, int local_dim_3, int offset_1, int offset_2, int offset_3);

/* runtime initialization/shutdown APIs */
extern CLU_API_ENTRY cl_int CLU_API_CALL
cluInitialize(clu_initialize_params* params); /* may be NULL */

extern CLU_API_ENTRY
cl_bool CLU_API_CALL cluIsInitialized(void);

extern CLU_API_ENTRY void CLU_API_CALL
cluRelease(void);

/* Device, Context, and Queue APIs */
extern CLU_API_ENTRY cl_device_id CLU_API_CALL
cluGetDevice(cl_device_type device_type);

extern CLU_API_ENTRY cl_context CLU_API_CALL
cluGetContext();

extern CLU_API_ENTRY cl_command_queue CLU_API_CALL
cluGetCommandQueue(cl_device_type device_type,
                   cl_int*        errcode_ret); /* may be NULL */

/* Kernel APIs */
extern CLU_API_ENTRY cl_int CLU_API_CALL
cluEnqueue(cl_kernel kern,
           clu_enqueue_params* params);

/* Get build errors (if any) from a program */
extern CLU_API_ENTRY const char * CLU_API_CALL
cluGetBuildErrors(cl_program program);

/* Build a program for all current devices from a string */
extern CLU_API_ENTRY cl_program CLU_API_CALL
cluBuildSource(const char* in_string,
               size_t      string_length,  /* may be NULL */
               cl_int*     errcode_ret);   /* may be NULL */

/* Build a program for all current devices from an array of strings */
extern CLU_API_ENTRY cl_program CLU_API_CALL
cluBuildSourceArray(cl_uint      num_strings,
                    const char** strings,
                    const char*  compile_options,  /* may be NULL */
                    cl_int*      errcode_ret);     /* may be NULL */

/* Loads a source file and builds a program for all current devices */
extern CLU_API_ENTRY cl_program CLU_API_CALL
cluBuildSourceFromFile(const char* file_name,
                       cl_int*     errcode_ret); /* may be NULL */

/* Allocate host memory aligned for optimal access and create a buffer using it */
/* Aligned memory will be freed automatically by clReleaseMemObject() via clSetMemObjectDestructorCallback() */
extern CLU_API_ENTRY cl_mem CLU_API_CALL
cluCreateAlignedBuffer(cl_mem_flags flags        /* 0 = read/write */,
                       size_t       size,
                       void**       out_host_ptr /* may be NULL */,
                       cl_int*      errcode_ret  /* may be NULL */);

/* wait until /any/ event in the list becomes CL_COMPLETE */
extern CLU_API_ENTRY cl_int CLU_API_CALL
cluWaitOnAnyEvent(const cl_event* event_list,
                  cl_uint         num_events);

/********************************************************************************************************/
/* APIs INLINEd for performance                                                                         */
/********************************************************************************************************/
INLINE CLU_API_ENTRY clu_nd_range CLU_API_CALL cluNDRange1(int global_dim_1, int local_dim_1, int offset_1)
{
    clu_nd_range range = {1, {(size_t) global_dim_1, 0, 0}, {(size_t) local_dim_1, 0, 0}, {(size_t) offset_1, 0, 0}};
    return range;
}

INLINE CLU_API_ENTRY clu_nd_range CLU_API_CALL cluNDRange2(int global_dim_1, int global_dim_2, int local_dim_1, int local_dim_2, int offset_1, int offset_2)
{
    clu_nd_range range = {2, {(size_t) global_dim_1, (size_t) global_dim_2, 0}, {(size_t) local_dim_1, (size_t) local_dim_2, 0}, {(size_t) offset_1, (size_t) offset_2, 0}};
    return range;
}

INLINE CLU_API_ENTRY clu_nd_range CLU_API_CALL cluNDRange3(int global_dim_1, int global_dim_2, int global_dim_3, int local_dim_1, int local_dim_2, int local_dim_3, int offset_1, int offset_2, int offset_3)
{
    clu_nd_range range = {3, {(size_t) global_dim_1, (size_t) global_dim_2, (size_t) global_dim_3}, {(size_t) local_dim_1, (size_t) local_dim_2, (size_t) local_dim_3}, {(size_t) offset_1, (size_t) offset_2, (size_t) offset_3}};
    return range;
}

INLINE CLU_API_ENTRY clu_enqueue_params cluGetDefaultParams()
{
    clu_enqueue_params p = {};
    return p;
}

/********************************************************************************************************/
/* Utility Functions                                                                                     */
/********************************************************************************************************/
#define CLU_UTIL_MAX_STRING_LENGTH 256
#define CLU_UTIL_PLATFORM_EXTENSION_ARRAY_SIZE (CLU_UTIL_MAX_STRING_LENGTH*128)

/* a struct describing the available CL platforms */
typedef struct
{
    char profile[CLU_UTIL_MAX_STRING_LENGTH];
    char version[CLU_UTIL_MAX_STRING_LENGTH];
    char name[CLU_UTIL_MAX_STRING_LENGTH];
    char vendor[CLU_UTIL_MAX_STRING_LENGTH];
    char extensions[CLU_UTIL_PLATFORM_EXTENSION_ARRAY_SIZE];
} clu_platform_info;

/* a struct describing the capabilities of a CL device */
typedef struct
{
    cl_device_type devType;
    cl_uint vendor_id;

    cl_uint max_compute_units;
    cl_uint max_work_item_dims;
    size_t max_work_items_per_dimension[3];
    size_t max_wg_size;
    
    cl_uint device_preferred_vector_width_char;
    cl_uint device_preferred_vector_width_short;
    cl_uint device_preferred_vector_width_int;
    cl_uint device_preferred_vector_width_long;
    cl_uint device_preferred_vector_width_float;
    cl_uint device_preferred_vector_width_half;

    cl_uint device_native_vector_width_char;
    cl_uint device_native_vector_width_short;
    cl_uint device_native_vector_width_int;
    cl_uint device_native_vector_width_long;
    cl_uint device_native_vector_width_float;
    cl_uint device_native_vector_width_half;

    cl_uint  max_clock_frequency;
    cl_uint  address_bits;
    cl_ulong max_mem_alloc_size;

    cl_bool image_support;
    cl_uint max_read_image_args;
    cl_uint max_write_image_args;
    size_t  image2d_max_width;
    size_t  image2d_max_height;
    size_t  image3d_max_width;
    size_t  image3d_max_height;
    size_t  image3d_max_depth;
    cl_uint max_samplers;
    
    size_t   max_parameter_size; 
    cl_uint  mem_base_addr_align;
    cl_uint  min_data_type_align_size;
    cl_device_fp_config      single_fp_config;
    cl_device_mem_cache_type global_mem_cache_type;
    cl_uint  global_mem_cacheline_size;
    cl_ulong global_mem_cache_size;
    cl_ulong global_mem_size;
    cl_ulong constant_buffer_size;
    cl_uint  max_constant_args;
    cl_device_local_mem_type local_mem_type;
    cl_ulong local_mem_size;
    cl_bool  error_correction_support;

    cl_bool unified_memory;
    size_t  profiling_timer_resolution;
    cl_bool endian_little;
    cl_bool device_available;
    cl_bool compiler_available;
    cl_device_exec_capabilities device_capabilities;
    cl_command_queue_properties queue_properties;
    cl_platform_id platform_id;

    char device_name[CLU_UTIL_MAX_STRING_LENGTH];
    char device_vendor[CLU_UTIL_MAX_STRING_LENGTH];
    char driver_version[CLU_UTIL_MAX_STRING_LENGTH];
    char device_profile[CLU_UTIL_MAX_STRING_LENGTH];
    char device_version[CLU_UTIL_MAX_STRING_LENGTH];
    char opencl_c_version[CLU_UTIL_MAX_STRING_LENGTH];
    char extensions[CLU_UTIL_PLATFORM_EXTENSION_ARRAY_SIZE];
} clu_device_info;

/* struct reflecting the flags supported by a given image format */
typedef struct
{
    cl_image_format pixelFormat;
    cl_mem_flags        supportedFlags; // union of supported flags
    cl_bool             supports2D;
    cl_bool             supports3D;
} clu_image_format;

/* Return a struct describing the available CL platforms */
extern CLU_API_ENTRY clu_platform_info CLU_API_CALL
cluGetPlatformInfo(cl_platform_id id, cl_int* errcode_ret); /* may be NULL */

/* return a struct describing the capabilities of a CL device */
extern CLU_API_ENTRY clu_device_info CLU_API_CALL
cluGetDeviceInfo(cl_device_id id,
                 cl_int* errcode_ret); /* may be NULL */

/* return an array of image formats supported in a given CL context */
/* the array returned is internal to CLU, applications should not attempt to free/delete it */
extern CLU_API_ENTRY const clu_image_format* CLU_API_CALL
cluGetSupportedImageFormats(cl_uint* array_size,
                            cl_int* errcode_ret); /* may be NULL */

/********************************************************************************************************/
/* String Functions: convert enums/defines to char*                                                     */
/********************************************************************************************************/

/* Return a string from an OpenCL return code, e.g. CL_SUCCESS returns "CL_SUCCESS" */
extern CLU_API_ENTRY const char* CLU_API_CALL
cluPrintError(cl_int error);

/* Return a string from image channel type */
extern CLU_API_ENTRY const char* CLU_API_CALL
cluPrintChannelType(cl_channel_type in_channelType);

/* Return a string from image channel order */
extern CLU_API_ENTRY const char* CLU_API_CALL
cluPrintChannelOrder(cl_channel_order in_channelOrder);

#ifdef __cplusplus
} // extern "C"
#endif

#endif //__CLU_H
