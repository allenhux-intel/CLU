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

/******************************************************************************/
 
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

/* Platform API */

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

/********************************************************************************************************/
/* APIs INLINEd for performance
/********************************************************************************************************/
INLINE CLU_API_ENTRY clu_nd_range CLU_API_CALL cluNDRange1(int global_dim_1, int local_dim_1, int offset_1)
{
    clu_nd_range range = {1, {global_dim_1, 0, 0}, {local_dim_1, 0, 0}, {offset_1, 0, 0}};
    return range;
}

INLINE CLU_API_ENTRY clu_nd_range CLU_API_CALL cluNDRange2(int global_dim_1, int global_dim_2, int local_dim_1, int local_dim_2, int offset_1, int offset_2)
{
    clu_nd_range range = {2, {global_dim_1, global_dim_2, 0}, {local_dim_1, local_dim_2, 0}, {offset_1, offset_2, 0}};
    return range;
}

INLINE CLU_API_ENTRY clu_nd_range CLU_API_CALL cluNDRange3(int global_dim_1, int global_dim_2, int global_dim_3, int local_dim_1, int local_dim_2, int local_dim_3, int offset_1, int offset_2, int offset_3)
{
    clu_nd_range range = {3, {global_dim_1, global_dim_2, global_dim_3}, {local_dim_1, local_dim_2, local_dim_3}, {offset_1, offset_2, offset_3}};
    return range;
}

INLINE CLU_API_ENTRY clu_enqueue_params cluGetDefaultParams()
{
    clu_enqueue_params p = {0};
    return p;
}

/********************************************************************************************************/
/* CLU Utility Funtions
/********************************************************************************************************/

/* Return a string describing the available CL platforms (not thread safe) */
extern CLU_API_ENTRY const char* CLU_API_CALL
cluGetPlatformInfo(cl_int* errcode_ret); /* may be NULL */

/* Return a string describing the available CL devices (not thread safe) */
extern CLU_API_ENTRY const char* CLU_API_CALL
cluGetDeviceInfo(cl_int* errcode_ret); /* may be NULL */

#ifdef __cplusplus
} // extern "C"
#endif

#endif //__CLU_H
