/*
Copyright (c) 2012, Intel Corporation

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#define CLU_MAGIC_BUILD_FLAG "CLU_GENERATED_BUILD"
#define CLU_MAXPROPERTYCOUNT 64

#define CLU_DETECT_MEMORY_LEAKS 0
// enable memory leak detection on MS 2010 + DEBUG build
// note this memory detection code fails in VS 2008
#if (defined _MSC_VER) && (defined _DEBUG)
//#undef CLU_DETECT_MEMORY_LEAKS
//#define CLU_DETECT_MEMORY_LEAKS 1
#endif

#if CLU_DETECT_MEMORY_LEAKS

    #define _CRTDBG_MAP_ALLOC
    #include <stdlib.h>
    #include <crtdbg.h>

    #ifndef DBG_NEW
        #define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
        #define new DBG_NEW
    #endif

#endif // MS compiler && _DEBUG

#include <list>
#include <algorithm>
#include <fstream>
#include <assert.h>
#include <string>
#include <map>
#include <sstream>

#include <string.h> // gcc needs this for memset
#include "clu.h"

#ifdef _DEBUG
static const char* OpenCLErrorCodeToString(cl_uint err)
{
    switch (err) {
#define OPENCLERRORCODETOSTRING_CASE(X) case X : return #X;
    OPENCLERRORCODETOSTRING_CASE(CL_SUCCESS                        )
    OPENCLERRORCODETOSTRING_CASE(CL_DEVICE_NOT_FOUND               )
    OPENCLERRORCODETOSTRING_CASE(CL_DEVICE_NOT_AVAILABLE           )
    OPENCLERRORCODETOSTRING_CASE(CL_COMPILER_NOT_AVAILABLE         )
    OPENCLERRORCODETOSTRING_CASE(CL_MEM_OBJECT_ALLOCATION_FAILURE  )
    OPENCLERRORCODETOSTRING_CASE(CL_OUT_OF_RESOURCES               )
    OPENCLERRORCODETOSTRING_CASE(CL_OUT_OF_HOST_MEMORY             )
    OPENCLERRORCODETOSTRING_CASE(CL_PROFILING_INFO_NOT_AVAILABLE   )
    OPENCLERRORCODETOSTRING_CASE(CL_MEM_COPY_OVERLAP               )
    OPENCLERRORCODETOSTRING_CASE(CL_IMAGE_FORMAT_MISMATCH          )
    OPENCLERRORCODETOSTRING_CASE(CL_IMAGE_FORMAT_NOT_SUPPORTED     )
    OPENCLERRORCODETOSTRING_CASE(CL_BUILD_PROGRAM_FAILURE          )
    OPENCLERRORCODETOSTRING_CASE(CL_MAP_FAILURE                    )
    OPENCLERRORCODETOSTRING_CASE(CL_INVALID_VALUE                  )
    OPENCLERRORCODETOSTRING_CASE(CL_INVALID_DEVICE_TYPE            )
    OPENCLERRORCODETOSTRING_CASE(CL_INVALID_PLATFORM               )
    OPENCLERRORCODETOSTRING_CASE(CL_INVALID_DEVICE                 )
    OPENCLERRORCODETOSTRING_CASE(CL_INVALID_CONTEXT                )
    OPENCLERRORCODETOSTRING_CASE(CL_INVALID_QUEUE_PROPERTIES       )
    OPENCLERRORCODETOSTRING_CASE(CL_INVALID_COMMAND_QUEUE          )
    OPENCLERRORCODETOSTRING_CASE(CL_INVALID_HOST_PTR               )
    OPENCLERRORCODETOSTRING_CASE(CL_INVALID_MEM_OBJECT             )
    OPENCLERRORCODETOSTRING_CASE(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR)
    OPENCLERRORCODETOSTRING_CASE(CL_INVALID_IMAGE_SIZE             )
    OPENCLERRORCODETOSTRING_CASE(CL_INVALID_SAMPLER                )
    OPENCLERRORCODETOSTRING_CASE(CL_INVALID_BINARY                 )
    OPENCLERRORCODETOSTRING_CASE(CL_INVALID_BUILD_OPTIONS          )
    OPENCLERRORCODETOSTRING_CASE(CL_INVALID_PROGRAM                )
    OPENCLERRORCODETOSTRING_CASE(CL_INVALID_PROGRAM_EXECUTABLE     )
    OPENCLERRORCODETOSTRING_CASE(CL_INVALID_KERNEL_NAME            )
    OPENCLERRORCODETOSTRING_CASE(CL_INVALID_KERNEL_DEFINITION      )
    OPENCLERRORCODETOSTRING_CASE(CL_INVALID_KERNEL                 )
    OPENCLERRORCODETOSTRING_CASE(CL_INVALID_ARG_INDEX              )
    OPENCLERRORCODETOSTRING_CASE(CL_INVALID_ARG_VALUE              )
    OPENCLERRORCODETOSTRING_CASE(CL_INVALID_ARG_SIZE               )
    OPENCLERRORCODETOSTRING_CASE(CL_INVALID_KERNEL_ARGS            )
    OPENCLERRORCODETOSTRING_CASE(CL_INVALID_WORK_DIMENSION         )
    OPENCLERRORCODETOSTRING_CASE(CL_INVALID_WORK_GROUP_SIZE        )
    OPENCLERRORCODETOSTRING_CASE(CL_INVALID_WORK_ITEM_SIZE         )
    OPENCLERRORCODETOSTRING_CASE(CL_INVALID_GLOBAL_OFFSET          )
    OPENCLERRORCODETOSTRING_CASE(CL_INVALID_EVENT_WAIT_LIST        )
    OPENCLERRORCODETOSTRING_CASE(CL_INVALID_EVENT                  )
    OPENCLERRORCODETOSTRING_CASE(CL_INVALID_OPERATION              )
    OPENCLERRORCODETOSTRING_CASE(CL_INVALID_GL_OBJECT              )
    OPENCLERRORCODETOSTRING_CASE(CL_INVALID_BUFFER_SIZE            )
    OPENCLERRORCODETOSTRING_CASE(CL_INVALID_MIP_LEVEL              )
    OPENCLERRORCODETOSTRING_CASE(CL_INVALID_GLOBAL_WORK_SIZE       )
#undef OPENCLERRORCODETOSTRING_CASE
    }
    return "Unknown CL error";
}

#define OCL_VALIDATE(r) \
{ \
    if (r != CL_SUCCESS) \
    {\
        fprintf(stderr, "%s(%d) error: returned %d (%s)\n", \
        __FILE__, __LINE__, r, OpenCLErrorCodeToString(r)); \
        /*assert(false); */\
    } \
}
#else
#define OCL_VALIDATE(in_status) (void)in_status
#endif

// cpu, gpu, accelerator, custom
#define CLU_MAX_NUM_DEVICES 4

//==============================================================================
// class to convert a cl_device_type to an index into internal array
//==============================================================================
class DeviceTypeToId
{
public:
    DeviceTypeToId();
    void SetDevice(cl_device_type in_t, cl_device_id in_id);
    void SetDefault(cl_device_type in_t);
    cl_device_id GetDevice(cl_device_type in_t);
    int GetIndexFromType(cl_device_type in_t);
private:
    int m_defaultDeviceIndex;
    cl_device_id m_deviceIds[CLU_MAX_NUM_DEVICES];
};

DeviceTypeToId::DeviceTypeToId()
{
    memset(m_deviceIds, 0, sizeof(m_deviceIds));
    m_defaultDeviceIndex = 0;
}

int DeviceTypeToId::GetIndexFromType(cl_device_type in_t)
{
    int index = 0;
    // FIXME: could use a bit scan...
    switch(in_t)
    {
    case CL_DEVICE_TYPE_CPU: index = 0; break;
    case CL_DEVICE_TYPE_GPU: index = 1; break;
    case CL_DEVICE_TYPE_ACCELERATOR: index = 2; break;
    default:
    case CL_DEVICE_TYPE_DEFAULT: index = m_defaultDeviceIndex; break;
    };
    return index;
}

void DeviceTypeToId::SetDevice(cl_device_type in_t, cl_device_id in_id)
{
    int index = GetIndexFromType(in_t);
    m_deviceIds[index] = in_id;
}

void DeviceTypeToId::SetDefault(cl_device_type in_t)
{
    m_defaultDeviceIndex = GetIndexFromType(in_t);
}

cl_device_id DeviceTypeToId::GetDevice(cl_device_type in_t)
{
    int index = GetIndexFromType(in_t);
    return m_deviceIds[index];
}

//==============================================================================
// class to maintain internal runtime state
//==============================================================================
class CLU_Runtime
{
public:
    static CLU_Runtime& Get() {return g_runtime;} // singleton pattern

    // startup
    cl_int       Initialize(const clu_initialize_params& in_params);

    // manage the lifetime of an object
    template<typename T> void AddObject(T o);

    // common build function
    cl_program   BuildProgram(cl_uint in_numStrings,
                              const char** in_strings, const size_t* in_string_lengths,
                              const char* in_buildOptions, cl_int *out_pStatus);
    const char*  GetBuildErrors(cl_program program);

    cl_device_id GetDevice(cl_device_type in_clDeviceType);
    cl_command_queue GetCommandQueue(cl_device_type in_clDeviceType, cl_int* out_status);
    cl_context   GetContext()                    {return m_context;}
    cl_bool      GetIsInitialized()              {if (m_isInitialized) return CL_TRUE; return CL_FALSE;}
    const char*  GetBuildOptions()               {return m_buildOptions.c_str();}

    // max buffer alignment across all devices in context
    cl_uint      GetBufferAlignment();

    // used by code generator: build and has program on first call,
    // subsequently return hashed program
    cl_program   HashProgram(cl_uint in_numStrings,
                              const char** in_strings, const size_t* in_string_lengths,
                              const char* in_buildOptions, cl_int *out_pStatus);

    void Reset(); // set everything to initial state, release all objects
private:
    CLU_Runtime();
    ~CLU_Runtime();

    static CLU_Runtime g_runtime;

    bool             m_isInitialized;
    cl_context       m_context; // default context
    cl_command_queue m_commandQueue[CLU_MAX_NUM_DEVICES];
    cl_command_queue_properties m_queueProperties;
    std::string      m_buildOptions;
    cl_uint          m_bufferAlignment; // max buffer alignment across all devices in context

    //---------------------------------------------------------------
    // any object allocated by runtime is managed by the runtime
    // Create a list of polymorphic objects:
    class CLU_Object
    {
    public:
        virtual ~CLU_Object() {}
    };
    std::list<CLU_Object*> m_objects;
    // Specific implementations of the object call CL Release methods:
    typedef std::list<CLU_Object*>::iterator cluObjectIterator;
    template<typename T> class CLU_Specific : public CLU_Object
    {
    protected:
        T m_o;
    public:
        CLU_Specific(T o) : m_o(o) {}
        virtual ~CLU_Specific() {assert(0);} // default destructor should never be called
    };
    //---------------------------------------------------------------

    //---------------------------------------------------------------
    // Devices and map of devices to device type
    // support up to 4 queues (1 for each of 4 devices: cpu, gpu, accelerator, custom)
    cl_uint          m_numDevices;
    
    // WARNING: do not try to use this with cl_device_type!
    // it is filled in at context creation time
    // it is used for building programs and returning build errors
    cl_device_id     m_deviceIds[CLU_MAX_NUM_DEVICES];

    DeviceTypeToId   m_device_type_to_id;
    //---------------------------------------------------------------

    // storage for objects created by generated code
    std::map<const void* const, cl_program> m_programMap;
};

//-----------------------------------------------------------------------------
// overloaded template definitions for the virtual destructors for
// all the kinds of things that the runtime might hold internal references to:
//-----------------------------------------------------------------------------
template<> CLU_Runtime::CLU_Specific<cl_context>::~CLU_Specific()       {clReleaseContext(m_o);}
template<> CLU_Runtime::CLU_Specific<cl_command_queue>::~CLU_Specific() {clReleaseCommandQueue(m_o);}
template<> CLU_Runtime::CLU_Specific<cl_program>::~CLU_Specific()       {clReleaseProgram(m_o);}

//-----------------------------------------------------------------------------
// global object
//-----------------------------------------------------------------------------
CLU_Runtime CLU_Runtime::g_runtime;

//-----------------------------------------------------------------------------
// add an object to the internal collection of objects
//-----------------------------------------------------------------------------
template<typename T> void CLU_Runtime::AddObject(T t)
{
    CLU_Object* p = new CLU_Specific<T>(t);
    m_objects.push_front(p);
}

//-----------------------------------------------------------------------------
// set everything to initial state, release all objects
//-----------------------------------------------------------------------------
void CLU_Runtime::Reset()
{
    m_context=0;
    m_numDevices=0;
    m_queueProperties = 0;
    m_bufferAlignment = 0;
    m_buildOptions.clear();
    memset(m_commandQueue, 0, sizeof(m_commandQueue));
    memset(m_deviceIds, 0, sizeof(m_deviceIds));

    for (cluObjectIterator i = m_objects.begin(); i != m_objects.end(); i++)
    {
        CLU_Object* p = *i;
        delete(p);
    }

    m_objects.clear();
    m_programMap.clear();

    m_isInitialized = false;
}

//-----------------------------------------------------------------------------
// constructor
//-----------------------------------------------------------------------------
CLU_Runtime::CLU_Runtime()
{
    //_CrtSetBreakAlloc(237); // set this to # of leaked allocation
    Reset();
}

//-----------------------------------------------------------------------------
// shutdown clu. Release all internal objects
//-----------------------------------------------------------------------------
CLU_Runtime::~CLU_Runtime()
{
    Reset();
#if CLU_DETECT_MEMORY_LEAKS
    _CrtDumpMemoryLeaks();
#endif
}

//-----------------------------------------------------------------------------
// internal method to return default platform
// find the first platform that supports the requested device type(s)
//-----------------------------------------------------------------------------
cl_platform_id GetPlatformDefault(
    int in_numPlatforms, const cl_platform_id* in_platforms,
    cl_device_type in_device_type)
{
    int i = 0;
    for (; i < in_numPlatforms; i++)
    {
        cl_device_id unused = 0;
        cl_uint numDevicesFound = 0;
        clGetDeviceIDs(in_platforms[i], in_device_type, 1, &unused, &numDevicesFound);
        if (0 != numDevicesFound)
        {
            break; // found platform with requested device type
        }
    }
    return in_platforms[i];
}

//-----------------------------------------------------------------------------
// internal method to get a platform by vendor name string
//-----------------------------------------------------------------------------
cl_platform_id GetPlatformByVendor(cl_uint in_numPlatforms, const cl_platform_id* in_platforms,
    const char* in_vendorName, cl_int* out_pStatus)
{
    const int BUFSIZE = 1024;
    char pBuf[BUFSIZE];
    cl_platform_id platform = 0;

    cl_uint i = 0;
    for (; i < in_numPlatforms; ++i) 
    {
        cl_int status = clGetPlatformInfo(in_platforms[i], CL_PLATFORM_VENDOR, BUFSIZE, pBuf, 0);
        OCL_VALIDATE(status);
        *out_pStatus = status;

        // just get part of the vendor string right
        if (0!=strstr(pBuf, in_vendorName))
        {
            platform = in_platforms[i];
        }
    }
    return platform;
}

//-----------------------------------------------------------------------------
// Initialize (start) clu
//-----------------------------------------------------------------------------
cl_int CLU_Runtime::Initialize(const clu_initialize_params& in_params)
{
    // if already initialized, do nothing.
    if (GetIsInitialized())
    {
        return CL_INVALID_OPERATION;
    }

    Reset();
    cl_platform_id* platforms = 0;

    m_queueProperties = in_params.default_queue_props;
    //m_queueProperties |= CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE;
    m_buildOptions = in_params.compile_options ? in_params.compile_options : "";
    m_context = in_params.existing_context;
    cl_device_type deviceType = in_params.preferred_device_type;
    if (0 == deviceType)
    {
        deviceType = CL_DEVICE_TYPE_ALL;
    }

    cl_int status = 0;
    if (0 == m_context)
    {
        // create default context and default queues (per device)
        // find an OCL device on this platform
        cl_uint numPlatforms;
        status = clGetPlatformIDs(0, 0, &numPlatforms);
        OCL_VALIDATE(status);
        if (CL_SUCCESS != status) goto exit;

        assert(0 != numPlatforms); // if it's 0, we should have gotten an error above.

        // get the number of platforms
        platforms = new cl_platform_id[numPlatforms];
        if (0 == platforms)
        {
            status = CL_OUT_OF_HOST_MEMORY;
            goto exit;
        }
        // get the platform ids
        status = clGetPlatformIDs(numPlatforms, platforms, 0);
        OCL_VALIDATE(status);
        if (CL_SUCCESS != status) goto exit;

        cl_platform_id platform = 0;
        if (in_params.vendor_name)
        {
            platform = GetPlatformByVendor(numPlatforms, platforms, in_params.vendor_name, &status);
            OCL_VALIDATE(status);
            if (CL_SUCCESS != status) goto exit;
        }

        if (0 == platform)
        {
            platform = GetPlatformDefault(numPlatforms, platforms, deviceType);
        }

        // should have a platform now.
        assert(platform);

        // get # of devices & device Ids
        // here we have a platform, no context yet
        status = clGetDeviceIDs(platform, deviceType, 0, 0, &m_numDevices);
        OCL_VALIDATE(status);
        if (CL_SUCCESS != status) goto exit;

        // get device IDs
        // clamp to CLU_MAX_NUM_DEVICES
        if (m_numDevices > CLU_MAX_NUM_DEVICES)
        {
            m_numDevices = CLU_MAX_NUM_DEVICES;
        }
        status = clGetDeviceIDs(platform, deviceType, m_numDevices, m_deviceIds, 0);
        OCL_VALIDATE(status);
        if (CL_SUCCESS != status) goto exit;

        // NOTE: GCC complains if we use array initialisers combined with goto,
        //       so do element-wise assignment instead
        cl_context_properties properties[CLU_MAXPROPERTYCOUNT];
        int propNum = 0;

        // append any user-provided context properties
        if (in_params.default_context_props)
        {
            // leave room for platform # and trailing 0
            int maxPropertyCount = CLU_MAXPROPERTYCOUNT - 3;
            while (propNum < maxPropertyCount)
            {
                cl_context_properties prop = in_params.default_context_props[propNum];
                if (0 == prop)
                {
                    break;
                }
                else
                {
                    properties[propNum] = prop;
                    ++propNum;
                    properties[propNum] = in_params.default_context_props[propNum];
                    ++propNum;
                }
            } // end loop over input properties
        } // end if there are default context properties
        properties[propNum] = CL_CONTEXT_PLATFORM;
        ++propNum;
        properties[propNum] = (cl_context_properties)platform;
        ++propNum;
        properties[propNum] = (cl_context_properties)0;

        m_context = clCreateContext(properties, m_numDevices, m_deviceIds,
            0 /* callback */, 0 /* callback data */, &status);
        OCL_VALIDATE(status);
        if (CL_SUCCESS != status) goto exit;
    }
    else
    {
        clRetainContext(in_params.existing_context); // add a reference to the provided context

        // get # of devices & device Ids
        // here, we have the context, not the platform
        cl_int status = clGetContextInfo(m_context, CL_CONTEXT_NUM_DEVICES, sizeof(m_numDevices), &m_numDevices, 0);
        OCL_VALIDATE(status);
        if (CL_SUCCESS != status) goto exit;

        // clamp to CLU_MAX_NUM_DEVICES
        if (m_numDevices > CLU_MAX_NUM_DEVICES)
        {
            m_numDevices = CLU_MAX_NUM_DEVICES;
        }
        status = clGetContextInfo(m_context, CL_CONTEXT_DEVICES, m_numDevices * sizeof(cl_device_id), m_deviceIds, 0);
        OCL_VALIDATE(status);
        if (CL_SUCCESS != status) goto exit;
    }

    // construct a map from clu device order (cpu, gpu, accelerator, custom) into platform device order
    for (cl_uint d = 0; d < m_numDevices; d++)
    {
        cl_device_type deviceType;
        status = clGetDeviceInfo(m_deviceIds[d], CL_DEVICE_TYPE, sizeof(cl_device_type), &deviceType, 0);
        OCL_VALIDATE(status);

        // usually device 0 is the default, but maybe this runtime explicitly sets this bit:
        if ((0 == d) || (deviceType & CL_DEVICE_TYPE_DEFAULT))
        {
            deviceType &= ~CL_DEVICE_TYPE_DEFAULT; // clear the bit
            m_device_type_to_id.SetDefault(deviceType);
        }

        m_device_type_to_id.SetDevice(deviceType, m_deviceIds[d]);
    }

    AddObject(m_context);

    m_isInitialized = true;

exit:
    delete [] platforms;
    return status;
}

//------------------------------------------------------------------------
// return cl_device from in_clDeviceType
//------------------------------------------------------------------------
cl_device_id CLU_Runtime::GetDevice(cl_device_type in_clDeviceType)
{
    return m_device_type_to_id.GetDevice(in_clDeviceType);
}

//-----------------------------------------------------------------------------
// return command queue based on 
//-----------------------------------------------------------------------------
cl_command_queue CLU_Runtime::GetCommandQueue(cl_device_type in_clDeviceType, cl_int* out_status)
{
    cl_int status = CL_SUCCESS;
    int index = m_device_type_to_id.GetIndexFromType(in_clDeviceType);
    cl_command_queue q = m_commandQueue[index];
    if (0 == q)
    {
        cl_device_id deviceId = m_device_type_to_id.GetDevice(in_clDeviceType);
        q = clCreateCommandQueue(m_context, deviceId, m_queueProperties, &status);
        OCL_VALIDATE(status);

        if (q)
        {
            m_commandQueue[index] = q;
            AddObject(q);
        }
    }

    if (out_status)
    {
        *out_status = status;
    }
    return q;
}

//-----------------------------------------------------------------------------
// return build errors
// NOT THREAD SAFE: this uses an internal char* for convenience
//-----------------------------------------------------------------------------
const char* CLU_Runtime::GetBuildErrors(cl_program in_program)
{
    static std::string buildString;
    buildString.clear();
    for (cl_uint d = 0; d < m_numDevices; d++)
    {
        // get build log size
        size_t buildLogSize = 0;
        cl_int status = clGetProgramBuildInfo(
            in_program, m_deviceIds[d], CL_PROGRAM_BUILD_LOG,
            0, 0, &buildLogSize);
        OCL_VALIDATE(status);

        char* buildLog = new char[buildLogSize+1];
        assert(buildLog);

        // get the build log
        status = clGetProgramBuildInfo(
            in_program, m_deviceIds[d], CL_PROGRAM_BUILD_LOG, 
            buildLogSize, buildLog, 0);
        OCL_VALIDATE(status);

        buildLog[buildLogSize] = 0;
        buildString.append(buildLog);
        delete [] buildLog;
    }
    return buildString.c_str();
}

//-----------------------------------------------------------------------------
// Build a program, called by generated code
//-----------------------------------------------------------------------------
cl_program CLU_Runtime::HashProgram(
    cl_uint in_numStrings, const char** in_strings, const size_t* in_string_lengths,
    const char* in_buildOptions,
    cl_int*     out_pStatus)
{
    cl_int status = CL_SUCCESS;

    // because this is called by generated code, we can be confident that the
    // string passed in is at an address that is constant for the lifetime of
    // the application. Hence, it's suitable for use as a hash key:
    const void* hashKey = in_strings;

    cl_program program = m_programMap[hashKey]; // find program in hash
    if (0 == program) // hasn't been built yet?
    {
        program = BuildProgram(in_numStrings, in_strings, in_string_lengths, in_buildOptions, &status);
        if (program) // if successful
        {
            AddObject(program); // manage lifetime
            m_programMap[hashKey] = program; // add to hash
        }
    }

    if (out_pStatus)
    {
        *out_pStatus = status;
    }
    return program;
}

//-----------------------------------------------------------------------------
// Build a program
//-----------------------------------------------------------------------------
cl_program CLU_Runtime::BuildProgram(
    cl_uint in_numStrings, const char** in_strings, const size_t* in_string_lengths,
    const char* in_buildOptions,
    cl_int*     out_pStatus)
{
    cl_int status = CL_SUCCESS;

    cl_context context = GetContext();
    cl_program program = clCreateProgramWithSource(context, in_numStrings, in_strings, in_string_lengths, &status);
    OCL_VALIDATE(status);

    status = clBuildProgram(program, m_numDevices, m_deviceIds, in_buildOptions, 0, 0);
    OCL_VALIDATE(status);

    if (out_pStatus)
    {
        *out_pStatus = status;
    }

    return program;
}

//-----------------------------------------------------------------------------
// find max buffer alignment across all devices in context
//-----------------------------------------------------------------------------
cl_uint CLU_Runtime::GetBufferAlignment()
{
    if (0 == m_bufferAlignment)
    {
        cl_uint numDevices = 0;
        clGetContextInfo(CLU_CONTEXT, CL_CONTEXT_NUM_DEVICES, sizeof(cl_uint), &numDevices, 0);
        cl_device_id* pDevices = new cl_device_id[numDevices];
        clGetContextInfo(CLU_CONTEXT, CL_CONTEXT_DEVICES, sizeof(cl_device_id)*numDevices, pDevices, 0);
        for (cl_uint i = 0; i < numDevices; i++)
        {
            cl_uint deviceAlign = 0;
            clGetDeviceInfo(pDevices[i], CL_DEVICE_MEM_BASE_ADDR_ALIGN, sizeof(cl_uint), &deviceAlign, 0);
            if (deviceAlign > m_bufferAlignment)
            {
                m_bufferAlignment = deviceAlign;
            }
        }
        delete [] pDevices;
    }
    return m_bufferAlignment;
}

//**********************************************************************************
// clu API (extern "C")
//**********************************************************************************

//-----------------------------------------------------------------------------
// Start using clu
//-----------------------------------------------------------------------------
cl_int CLU_API_CALL
cluInitialize(clu_initialize_params* params)
{
    try
    {
        clu_initialize_params defaultParams = {0, 0, 0, 0, 0, CL_DEVICE_TYPE_ALL};
        if (params)
        {
            clu_initialize_params temp = {
                params->vendor_name, params->existing_context,
                params->compile_options, params->default_queue_props,
                params->default_context_props, params->preferred_device_type};
            defaultParams = temp;
        }
        return CLU_Runtime::Get().Initialize(defaultParams);
    }
    catch (...) // internal error, e.g. thrown by STL
    {
        return CL_DEVICE_NOT_FOUND;
    }
}

//-----------------------------------------------------------------------------
// has clu been initialized?
//-----------------------------------------------------------------------------
cl_bool CLU_API_CALL
    cluIsInitialized(void)
{
    try
    {
        return CLU_Runtime::Get().GetIsInitialized();
    }
    catch (...) // internal error, e.g. thrown by STL
    {
        return false;
    }
}

//-----------------------------------------------------------------------------
// release all memory associated with clu
//-----------------------------------------------------------------------------
void CLU_API_CALL
cluRelease(void)
{
    try
    {
        CLU_Runtime::Get().Reset();
    }
    catch (...) // internal error, e.g. thrown by STL
    {}
}

//-----------------------------------------------------------------------------
// return the runtime context
//   may have been created by the runtime, or provided with cluInitialize
//-----------------------------------------------------------------------------
cl_context CLU_API_CALL cluGetContext()
{
    return CLU_Runtime::Get().GetContext();
}

//-----------------------------------------------------------------------------
// return cl device associated with device type
//-----------------------------------------------------------------------------
cl_device_id CLU_API_CALL
cluGetDevice(cl_device_type in_clDeviceType)
{
    return CLU_Runtime::Get().GetDevice(in_clDeviceType);
}

//-----------------------------------------------------------------------------
// Return cl_command_queue associated with cl_device_id
//-----------------------------------------------------------------------------
cl_command_queue CLU_API_CALL
cluGetCommandQueue(cl_device_type in_clDeviceType,
                  cl_int * errcode_ret)
{
    return CLU_Runtime::Get().GetCommandQueue(in_clDeviceType, errcode_ret);
}

//-----------------------------------------------------------------------------
// enqueue a kernel
//-----------------------------------------------------------------------------
cl_int CLU_API_CALL
cluEnqueue(cl_kernel kern, clu_enqueue_params* params)
{
    cl_command_queue q = params->queue;
    if (0 == q)
    {
        q = CLU_DEFAULT_Q;
    }
    const clu_nd_range& range = params->nd_range;
    const size_t * offset = (!range.offset[0] && !range.offset[1] && !range.offset[2]) ? 0 : range.offset;
    const size_t * local  = (!range.local[0]  && !range.local[1]  && !range.local[2])  ? 0 : range.local;
    cl_int status = clEnqueueNDRangeKernel(q, kern, range.dim, offset, range.global, local,
        params->num_events_in_wait_list, params->event_wait_list, params->out_event);
    return status;
}

//-----------------------------------------------------------------------------
// get build errors (as a string) from a cl_program
//-----------------------------------------------------------------------------
const char * CLU_API_CALL cluGetBuildErrors(cl_program in_program)
{
    const char* errors = 0;
    try
    {
        errors = CLU_Runtime::Get().GetBuildErrors(in_program);
    }
    catch (...)
    {
    }
    return errors;
}

//-----------------------------------------------------------------------------
// Build a program for all current devices from a string
//-----------------------------------------------------------------------------
cl_program CLU_API_CALL
cluBuildSource(const char* in_string,
              size_t string_length, /* may be NULL */
              cl_int * errcode_ret) /* may be NULL */
{
    cl_program program = 0;
    cl_int status = CL_INVALID_VALUE;

    size_t* pLength = 0;
    if (0 != string_length)
    {
        pLength = &string_length;
    }

    try
    {
        const char* buildOptions = CLU_Runtime::Get().GetBuildOptions();
        program = CLU_Runtime::Get().BuildProgram(1, &in_string, pLength, buildOptions, &status);
    }
    catch (...) // internal error, e.g. thrown by STL
    {
        status = CL_BUILD_PROGRAM_FAILURE;
    }

    if (errcode_ret)
    {
        *errcode_ret = status;
    }
    return program;
}

//-----------------------------------------------------------------------------
// Build a program for all current devices from a string
//   can override global build options
//-----------------------------------------------------------------------------
cl_program CLU_API_CALL
cluBuildSourceArray(cl_uint num_strings,
                   const char** strings,
                   const char*  compile_options, /* may be NULL */
                   cl_int * errcode_ret)         /* may be NULL */
{
    cl_program program = 0;
    cl_int status = CL_INVALID_VALUE;
    try
    {
        // was this build called by the clu code generator?
        // if so, we should be smart about handling the program object
        if (0 == strncmp(CLU_MAGIC_BUILD_FLAG, compile_options, strlen(CLU_MAGIC_BUILD_FLAG)))
        {
            compile_options = CLU_Runtime::Get().GetBuildOptions();
            // this will retrieve it from a hash or build it if it's not there:
            program = CLU_Runtime::Get().HashProgram(num_strings, strings, 0,
                compile_options, &status);
        }
        else
        {
            if (0 == compile_options)
            {
                compile_options = CLU_Runtime::Get().GetBuildOptions();
            }
            program = CLU_Runtime::Get().BuildProgram(num_strings, strings, 0,
                compile_options, &status);
        }
    }
    catch (...) // internal error, e.g. thrown by STL
    {
        status = CL_BUILD_PROGRAM_FAILURE;
    }

    if (errcode_ret)
    {
        *errcode_ret = status;
    }
    return program;
}

//-----------------------------------------------------------------------------
// convert a source file into a cl_program
//-----------------------------------------------------------------------------
cl_program CLU_API_CALL
cluBuildSourceFromFile(const char* in_pFileName, cl_int * errcode_ret)
{
    cl_int status = CL_INVALID_VALUE;
    cl_program program = 0;
    try
    {
        if (in_pFileName)
        {
            std::ifstream ifs(in_pFileName);
            if (ifs.is_open())
            {
                std::string s((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
                program = cluBuildSource(s.c_str(), s.size(), &status);
                ifs.close();
            }
        } // end if non-null file name string
    }
    catch (...) // internal error, e.g. thrown by STL
    {
        status = CL_OUT_OF_HOST_MEMORY;
    }

    if (errcode_ret)
    {
        *errcode_ret = status;
    }
    return program;
}

//=============================================================================
//utility functions and structs
//=============================================================================

#define CLU_UTIL_MAX_STRING_LENGTH 256
#define CLU_UTIL_PLATFORM_EXTENSION_ARRAY_SIZE (CLU_UTIL_MAX_STRING_LENGTH*128)

// conveniently convert int, etc. to std::string
template<typename T> std::string cluToString(T x)
{
    std::stringstream i;
    i << x;
    return i.str();
}

// cl_bool isn't sufficiently distinct to use an overloaded cluToString<>
#define BoolToStr(in_b) std::string(CL_TRUE==in_b?"true":"false")

//-----------------------------------------------------------------------------
// utility to acquire platform information string for all platforms.
//-----------------------------------------------------------------------------
const char* CLU_GetPlatformInfo(cl_int* out_pStatus)
{
    struct PlatformInfo
    {
        char platformProfile[CLU_UTIL_MAX_STRING_LENGTH];
        char platformVersion[CLU_UTIL_MAX_STRING_LENGTH];
        char platformName[CLU_UTIL_MAX_STRING_LENGTH];
        char platformVendor[CLU_UTIL_MAX_STRING_LENGTH];
        char platformExtensions[CLU_UTIL_PLATFORM_EXTENSION_ARRAY_SIZE];
    } platformInfo;

    static std::string s;
    s.clear();

    cl_uint numPlatforms = 0;
    cl_int status = clGetPlatformIDs(0, 0, &numPlatforms);
    OCL_VALIDATE(status);

    cl_platform_id* platforms = new cl_platform_id[numPlatforms];
    status = clGetPlatformIDs(numPlatforms, platforms, 0);
    OCL_VALIDATE(status);

    s += "-----------------------------------------------------------------------\n";
    s += "\t\tTHIS SYSTEM HAS " + cluToString(numPlatforms) + " OPENCL PLATFORM(S) INSTALLED\n";
    s += "-----------------------------------------------------------------------\n\n";

    s += "***********************************************************************\n";
    s += "\t\tBegin Platform Information\n";
    s += "***********************************************************************\n";    
    
    for(unsigned int i=0;i<numPlatforms;i++)
    {
        status = clGetPlatformInfo(platforms[i], CL_PLATFORM_PROFILE,    CLU_UTIL_MAX_STRING_LENGTH, platformInfo.platformProfile, 0);
        status = clGetPlatformInfo(platforms[i], CL_PLATFORM_VERSION,    CLU_UTIL_MAX_STRING_LENGTH, platformInfo.platformVersion, 0);
        status = clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME,       CLU_UTIL_MAX_STRING_LENGTH, platformInfo.platformName,    0);
        status = clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR,     CLU_UTIL_MAX_STRING_LENGTH, platformInfo.platformVendor,  0);
        status = clGetPlatformInfo(platforms[i], CL_PLATFORM_EXTENSIONS, CLU_UTIL_PLATFORM_EXTENSION_ARRAY_SIZE, platformInfo.platformExtensions, 0);

        s += "***********************************************************************\n";
        s += "\t\tDetails for platform " + cluToString(i+1) + " of " + cluToString(numPlatforms) + ": " + platformInfo.platformVendor + "\n";
        s += "***********************************************************************\n";
        s += "Platform Name: ";
        s += platformInfo.platformName;
        s += "\nVendor: ";
        s += platformInfo.platformVendor;
        s += "\nPlatform Profile: ";
        s += platformInfo.platformProfile;
        s += "\nVersion: ";
        s +=  platformInfo.platformVersion;
        s += "\nList of extensions supported:\n ";
        s += platformInfo.platformExtensions;
        s += "\n";
    }

    s += "***********************************************************************\n";
    s += "\t\tEnd Platform Information\n";
    s += "***********************************************************************\n";    
    
    delete [] platforms;
    
    if (out_pStatus)
    {
        *out_pStatus = status;
    }

    return s.c_str();
}

const char* CLU_API_CALL cluGetPlatformInfo(cl_int* out_pStatus)
{
    const char* info = 0;
    cl_int status;
    try
    {
        info = CLU_GetPlatformInfo(&status);
    }
    catch (...) // internal error, e.g. thrown by STL
    {
        status = CL_OUT_OF_HOST_MEMORY;
    }
    if (out_pStatus)
    {
        *out_pStatus = status;
    }
    return info;
}

//-----------------------------------------------------------------------------
// utility to acquire device information string for all devices in all platforms.
//-----------------------------------------------------------------------------
struct DeviceInfo
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

    cl_uint max_clock_frequency;
    cl_uint address_bits;
    cl_ulong max_mem_alloc_size;

    cl_bool image_support;
    cl_uint max_read_image_args;
    cl_uint    max_write_image_args;
    size_t image2d_max_width;
    size_t image2d_max_height;
    size_t image3d_max_width;
    size_t image3d_max_height;
    size_t image3d_max_depth;
    cl_uint max_samplers;
    
    size_t max_parameter_size; 
    cl_uint mem_base_addr_align;
    cl_uint min_data_type_align_size;
    cl_device_fp_config single_fp_config;
    cl_device_mem_cache_type global_mem_cache_type;
    cl_uint global_mem_cacheline_size;
    cl_ulong global_mem_cache_size;
    cl_ulong global_mem_size;
    cl_ulong constant_buffer_size;
    cl_uint max_constant_args;
    cl_device_local_mem_type local_mem_type;
    cl_ulong local_mem_size;
    cl_bool error_correction_support;

    cl_bool unified_memory;
    size_t profiling_timer_resolution;
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
};

const char* CLU_GetDeviceInfo(cl_int* out_pStatus)
{
    static std::string s;
    s.clear();

    int status;
    cl_uint numPlatforms = 0;
    
    //first we need to get the number of devices on the platform
    status = clGetPlatformIDs(0, 0, &numPlatforms);
    OCL_VALIDATE(status);

    cl_platform_id* platforms = new cl_platform_id[numPlatforms];

    status = clGetPlatformIDs(numPlatforms, platforms, 0);
    OCL_VALIDATE(status);

    s += "***********************************************************************\n";
    s += "\t\tBegin Per Platform Device Information\n";
    s += "***********************************************************************\n";    

    //then we can ask about their properties
    for(cl_uint i=0;i<numPlatforms;i++)
    {

        //get the number of devices for this platform
        cl_uint numDevices = 0;

        //frist, query this platform for the number of devices
        
        status = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, 0, 0, &numDevices);
        OCL_VALIDATE(status);

        cl_device_id* localDevices = new cl_device_id[numDevices];
        status = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, numDevices, localDevices, 0);
        OCL_VALIDATE(status);

        for(cl_uint j=0;j<numDevices;j++)
        {
            DeviceInfo deviceInfo;
            clGetDeviceInfo(localDevices[j], CL_DEVICE_TYPE, sizeof(cl_device_type), &deviceInfo.devType, 0);
            

            clGetDeviceInfo(localDevices[j], CL_DEVICE_VENDOR_ID, sizeof(cl_device_type), &deviceInfo.vendor_id, 0);

            clGetDeviceInfo(localDevices[j], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &deviceInfo.max_compute_units, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(cl_uint), &deviceInfo.max_work_item_dims, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_MAX_WORK_ITEM_SIZES, (sizeof(size_t)*3), &deviceInfo.max_work_items_per_dimension, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(cl_uint), &deviceInfo.max_wg_size, 0);

            clGetDeviceInfo(localDevices[j], CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR, sizeof(size_t), &deviceInfo.device_preferred_vector_width_char, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT, sizeof(cl_uint), &deviceInfo.device_preferred_vector_width_short, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT, sizeof(cl_uint), &deviceInfo.device_preferred_vector_width_int, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG, sizeof(cl_uint), &deviceInfo.device_preferred_vector_width_long, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT, sizeof(cl_uint), &deviceInfo.device_preferred_vector_width_float, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF, sizeof(cl_uint), &deviceInfo.device_preferred_vector_width_half, 0);
            
            clGetDeviceInfo(localDevices[j], CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR, sizeof(size_t), &deviceInfo.device_native_vector_width_char, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT, sizeof(cl_uint), &deviceInfo.device_native_vector_width_short, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_NATIVE_VECTOR_WIDTH_INT, sizeof(cl_uint), &deviceInfo.device_native_vector_width_int, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG, sizeof(cl_uint), &deviceInfo.device_native_vector_width_long, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT, sizeof(cl_uint), &deviceInfo.device_native_vector_width_float, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF, sizeof(cl_uint), &deviceInfo.device_native_vector_width_half, 0);
            
            clGetDeviceInfo(localDevices[j], CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(cl_uint), &deviceInfo.max_clock_frequency, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_ADDRESS_BITS, sizeof(cl_uint), &deviceInfo.address_bits, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(cl_ulong), &deviceInfo.max_mem_alloc_size, 0);
            
            clGetDeviceInfo(localDevices[j], CL_DEVICE_IMAGE_SUPPORT, sizeof(cl_bool), &deviceInfo.image_support, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_MAX_READ_IMAGE_ARGS, sizeof(cl_uint), &deviceInfo.max_read_image_args, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_MAX_WRITE_IMAGE_ARGS, sizeof(cl_uint), &deviceInfo.max_write_image_args, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_IMAGE2D_MAX_WIDTH, sizeof(size_t), &deviceInfo.image2d_max_width, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_IMAGE2D_MAX_HEIGHT, sizeof(size_t), &deviceInfo.image2d_max_height, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_IMAGE3D_MAX_WIDTH, sizeof(size_t), &deviceInfo.image3d_max_width, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_IMAGE3D_MAX_HEIGHT, sizeof(size_t), &deviceInfo.image3d_max_height, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_IMAGE3D_MAX_DEPTH, sizeof(size_t), &deviceInfo.image3d_max_depth, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_MAX_SAMPLERS, sizeof(cl_uint), &deviceInfo.max_samplers, 0);

            clGetDeviceInfo(localDevices[j], CL_DEVICE_MAX_PARAMETER_SIZE, sizeof(size_t), &deviceInfo.max_parameter_size, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_MEM_BASE_ADDR_ALIGN, sizeof(cl_uint), &deviceInfo.mem_base_addr_align, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE, sizeof(cl_uint), &deviceInfo.min_data_type_align_size, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_SINGLE_FP_CONFIG, sizeof(cl_device_fp_config), &deviceInfo.single_fp_config, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_GLOBAL_MEM_CACHE_TYPE, sizeof(cl_device_mem_cache_type), &deviceInfo.global_mem_cache_type, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE, sizeof(cl_uint), &deviceInfo.global_mem_cacheline_size, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, sizeof(cl_ulong), &deviceInfo.global_mem_cache_size, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(cl_ulong), &deviceInfo.global_mem_size, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE, sizeof(cl_ulong), &deviceInfo.constant_buffer_size, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_MAX_CONSTANT_ARGS, sizeof(cl_uint), &deviceInfo.max_constant_args, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_LOCAL_MEM_TYPE, sizeof(cl_device_local_mem_type), &deviceInfo.local_mem_type, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), &deviceInfo.local_mem_size, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_ERROR_CORRECTION_SUPPORT, sizeof(cl_bool), &deviceInfo.error_correction_support, 0);
            
            clGetDeviceInfo(localDevices[j], CL_DEVICE_HOST_UNIFIED_MEMORY, sizeof(cl_bool), &deviceInfo.unified_memory, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_PROFILING_TIMER_RESOLUTION, sizeof(size_t), &deviceInfo.profiling_timer_resolution, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_ENDIAN_LITTLE, sizeof(cl_bool), &deviceInfo.endian_little, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_AVAILABLE, sizeof(cl_bool), &deviceInfo.device_available, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_COMPILER_AVAILABLE, sizeof(cl_bool), &deviceInfo.compiler_available, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_EXECUTION_CAPABILITIES, sizeof(cl_device_exec_capabilities), &deviceInfo.device_capabilities, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_QUEUE_PROPERTIES, sizeof(cl_command_queue_properties), &deviceInfo.queue_properties, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_PLATFORM, sizeof(cl_platform_id), &deviceInfo.platform_id, 0);
            
            clGetDeviceInfo(localDevices[j], CL_DEVICE_NAME, (sizeof(char)*256), &deviceInfo.device_name, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_VENDOR, (sizeof(char)*256), &deviceInfo.device_vendor, 0);
            clGetDeviceInfo(localDevices[j], CL_DRIVER_VERSION, (sizeof(char)*256), &deviceInfo.driver_version, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_PROFILE, (sizeof(char)*256), &deviceInfo.device_profile, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_VERSION, (sizeof(char)*256), &deviceInfo.device_version, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_OPENCL_C_VERSION, (sizeof(char)*256), &deviceInfo.opencl_c_version, 0);
            clGetDeviceInfo(localDevices[j], CL_DEVICE_EXTENSIONS, (sizeof(char)*256*128), &deviceInfo.extensions, 0);

            s += "***********************************************************************\n";
            s += "\t\tPlatform: " + cluToString(i) + ", Device " + cluToString(j+1) + " of " + cluToString(numDevices) + ", Type: ";

            if(deviceInfo.devType == CL_DEVICE_TYPE_CPU)
            {
                    s += " CPU";
            }
            else if(deviceInfo.devType == CL_DEVICE_TYPE_GPU)
            {
                    s += " GPU";
            }
            else if(deviceInfo.devType == CL_DEVICE_TYPE_ACCELERATOR)
            {
                    s += " Accelerator";
            }
            /* for OCL 1.2
            else if(deviceInfo[i].devType == CL_DEVICE_TYPE_CUSTOM)
            {
                    s += " Custom\n";
            }
            */
            if(deviceInfo.devType == CL_DEVICE_TYPE_DEFAULT)
            {
                    s += " Default";
            }
            s += "\n***********************************************************************\n";
            s += "Device Name: ";
            s += deviceInfo.device_name;
            s += "\nVendor Name: ";
            s += deviceInfo.device_vendor;
            s += "\n\tDriver Version: ";
            s += deviceInfo.driver_version;
            s += "\n\tProfile: ";
            s += deviceInfo.device_profile;
            s += "\n\tDevice Version: ";
            s += deviceInfo.device_version;
            s += "\ntOpenCL C Version: ";
            s += deviceInfo.opencl_c_version;
            s += "\n\n";

            s += "Vendor ID: " + cluToString(deviceInfo.vendor_id) + "\n";

            s += "Max Compute Units: " + cluToString(deviceInfo.max_compute_units) + "\n";
            s += "Max # of Dimensions: " + cluToString(deviceInfo.max_work_item_dims) + "\n";

            s += "Max # items per dim: [" +
                cluToString(deviceInfo.max_work_items_per_dimension[0]) + "," +
                cluToString(deviceInfo.max_work_items_per_dimension[1]) + "," +
                cluToString(deviceInfo.max_work_items_per_dimension[2]) + "]\n";
            
            s += "Max workgroup size: "  + cluToString(deviceInfo.max_wg_size) + "\n";
            
            s += "\nPreferred Vector Widths:\n"
                "\tchar: "   + cluToString(deviceInfo.device_preferred_vector_width_char) +
                ", short: " + cluToString(deviceInfo.device_preferred_vector_width_short) +
                ", int: "   + cluToString(deviceInfo.device_preferred_vector_width_int) +
                ", long: "  + cluToString(deviceInfo.device_preferred_vector_width_long) +
                ", float: " + cluToString(deviceInfo.device_preferred_vector_width_float) +
                ", half: "  + cluToString(deviceInfo.device_native_vector_width_half) + "\n";
            
            s += "Native Vector Widths:\n"
                "\tchar: "   + cluToString(deviceInfo.device_native_vector_width_char) +
                ", short: " + cluToString(deviceInfo.device_native_vector_width_short) +
                ", int: "   + cluToString(deviceInfo.device_native_vector_width_int) +
                ", long: "  + cluToString(deviceInfo.device_native_vector_width_long) +
                ", float: " + cluToString(deviceInfo.device_native_vector_width_float) +
                ", half: "  + cluToString(deviceInfo.device_native_vector_width_half) + "\n";
            
            s += "\nMaximum Clock Freq: " + cluToString(deviceInfo.max_clock_frequency) + " Mhz\n";
            s += "Address Bits: " + cluToString(deviceInfo.address_bits) + "\n";
            s += "Max Memory Allocation Size: " + cluToString(deviceInfo.max_mem_alloc_size) + " bytes\n";

            s += "\nImages";
            s += "\n\tImage Support: " + BoolToStr(deviceInfo.image_support);
            s += "\n\tMax # of simultaneous read images: " + cluToString(deviceInfo.max_read_image_args);
            s += "\n\tMax # of simultaneous write images: " + cluToString(deviceInfo.max_write_image_args);
            s += "\n\t2D Max [W,H]: [" + cluToString(deviceInfo.image2d_max_width) + "," + cluToString(deviceInfo.image2d_max_height) + "]";
            s += "\n\t3D Max [W,H,D]:[" + cluToString(deviceInfo.image3d_max_width) + "," + cluToString(deviceInfo.image3d_max_height) + "," + cluToString(deviceInfo.image3d_max_depth) + "]";
            s += "\n\tMax # samplers: " + cluToString(deviceInfo.max_samplers) + "\n";
    
            s += "\nFP configuration\n";
            if(deviceInfo.single_fp_config & CL_FP_DENORM)
            {
                s += "\tDenorms Supported\n";
            }
            if(deviceInfo.single_fp_config & CL_FP_INF_NAN)
            {
                s += "\tINF and quiet NANs are supported\n";
            }
            if(deviceInfo.single_fp_config & CL_FP_ROUND_TO_NEAREST)
            {
                s += "\tRound to nearest even supported\n";
            }
            if(deviceInfo.single_fp_config & CL_FP_ROUND_TO_ZERO)
            {
                s += "\tRound to zero rounding mode supported\n";
            }
            if(deviceInfo.single_fp_config & CL_FP_ROUND_TO_INF)
            {
                s += "\tRound to positive and negative infinity rounding modes supported\n";
            }
            if(deviceInfo.single_fp_config & CL_FP_FMA)
            {
                s += "\tIEEE754-2008 fused multiply add supported\n";
            }
            if(deviceInfo.single_fp_config & CL_FP_SOFT_FLOAT)
            {
                s += "\tBasic floating point operations are implemented in software\n";
            }

            s += "\n";

            s += "Memory Properties\n";
            s += "\tMax size of argument that can be passed to kernel: " + cluToString(deviceInfo.max_parameter_size) + " bytes\n";
            s += "\tMin. value of largest datatype supported (For alignment): " + cluToString(deviceInfo.mem_base_addr_align) + " bits, " + cluToString(deviceInfo.min_data_type_align_size) + " bytes\n";
            s += "\tGlobal Memory:\n";
            s += "\t\tGlobal cacheline size: " + cluToString(deviceInfo.global_mem_cacheline_size) + " bytes\n";
            s += "\t\tGlobal Cache Size: " + cluToString(deviceInfo.global_mem_cache_size) + " bytes\n";
            s += "\t\tGlobal Device Memory: " + cluToString(deviceInfo.global_mem_size) + " bytes\n";
            
            s += "\t\tGlobal Memory Cache Type: ";
            if(deviceInfo.global_mem_cache_type == CL_NONE)
            {
                s += "None\n";
            }
            else if(deviceInfo.global_mem_cache_type == CL_READ_ONLY_CACHE)
            {
                s += "Read Only\n";
            }
            else if(deviceInfo.global_mem_cache_type == CL_READ_WRITE_CACHE)
            {
                s += "Read/Write\n";
            }

            s += "\tConstant Memory:\n";
            s += "\t\tConstant Buffer Size: " + cluToString(deviceInfo.constant_buffer_size) + " bytes\n";
            s += "\t\tMax number of constant arguments: " + cluToString(deviceInfo.max_constant_args) + "\n";
            s += "\tLocal Memory\n";
            s += "\t\tLocal Memory Size: " + cluToString(deviceInfo.local_mem_size) + " bytes\n";
            s += "\t\tError Correction Support: " + cluToString(deviceInfo.error_correction_support) + "\n";
            if(deviceInfo.local_mem_type == CL_LOCAL)
            { 
                s += "\t\tHas dedicated local memory storage\n";
            }
            else if(deviceInfo.local_mem_type == CL_GLOBAL)
            {
                s += "\t\tDoes not have Local memory, local memory is actually global\n";
            }

            s += "\tUnified Memory: " + BoolToStr(deviceInfo.unified_memory) + "\n";
            
            s += "\nProfiling Timer Resolution: " + cluToString(deviceInfo.profiling_timer_resolution);
            s += "\nLittle Endian: " + BoolToStr(deviceInfo.endian_little);
            s += "\nDevice Available: " + BoolToStr(deviceInfo.device_available);
            s += "\nCompiler Available: " + BoolToStr(deviceInfo.compiler_available);
            s += "\nPlatform Id: " + cluToString(deviceInfo.platform_id) + "\n";

            //start here
            s += "Device Execution Capabilities: \n";
            if(deviceInfo.device_capabilities & CL_EXEC_KERNEL)
            {
                s += "\tCan execute OpenCL kernels\n";
            }
            if(deviceInfo.device_capabilities & CL_EXEC_NATIVE_KERNEL)
            {
                s += "\tCan execute Native kernels\n";
            }
            
            s += "Device Queue Properties: \n";
            if(deviceInfo.queue_properties & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE)
            {
                s += "tCan execute OpenCL kernels out of order\n";
            }
            
            if(deviceInfo.queue_properties & CL_QUEUE_PROFILING_ENABLE)
            {
                s += "\tProfiling of queue commands currently ENABLED\n";
            }
            else
            {
                s += "\tProfiling of queue commands currently DISABLED\n";
            }

    
            s += "Extensions:\n";
            s += deviceInfo.extensions;
            s += "\n";


            s += "***********************************************************************\n";
            s += "******************** Platform: " + cluToString(i) + ", Device " + cluToString(j+1) + " End ************************\n";
            s += "***********************************************************************\n\n\n";

        } //end device loop

        delete [] localDevices;
    } //end platform loop


    s += "***********************************************************************\n";
    s += "\t\tEnd Per Platform Device Information\n";
    s += "***********************************************************************\n";    


    delete [] platforms;

    if (out_pStatus)
    {
        *out_pStatus = status;
    }

    return s.c_str();
}

const char* CLU_API_CALL cluGetDeviceInfo(cl_int* out_pStatus)
{
    const char* info = 0;
    cl_int status;
    try
    {
        info = CLU_GetDeviceInfo(&status);
    }
    catch (...) // internal error, e.g. thrown by STL
    {
        status = CL_OUT_OF_HOST_MEMORY;
    }
    if (out_pStatus)
    {
        *out_pStatus = status;
    }
    return info;
}

//-----------------------------------------------------------------------------
// release a buffer and its aligned host memory
// (expects created with cluCreateAlignedBuffer)
//-----------------------------------------------------------------------------
void CL_CALLBACK CLU_ReleaseAlignedBufferCallback(cl_mem in_buffer, void* in_pHostPtr)
{
    _aligned_free(in_pHostPtr);
}

//-----------------------------------------------------------------------------
// allocate host memory aligned for optimal access and create a buffer using it
//-----------------------------------------------------------------------------
cl_mem CLU_API_CALL cluCreateAlignedBuffer(
    cl_mem_flags in_flags,
    size_t in_size,
    void** out_pPtr,
    cl_int* out_pStatus)
{
    cl_mem mem = 0;
    try
    {
        cl_uint alignment = CLU_Runtime::Get().GetBufferAlignment();
        void* pMem = _aligned_malloc(in_size, alignment);
        if ((0 == pMem) && (out_pStatus))
        {
            *out_pStatus = CL_INVALID_VALUE;
        }
        else
        {
            mem = clCreateBuffer(CLU_CONTEXT, in_flags | CL_MEM_USE_HOST_PTR, in_size, pMem, out_pStatus);
        }
        if (mem)
        {
            cl_int status = clSetMemObjectDestructorCallback(mem, CLU_ReleaseAlignedBufferCallback, pMem);
            if (out_pPtr)
            {
                *out_pPtr = pMem;
            }
        }
        else
        {
            _aligned_free(pMem);
        }
    }
    catch (...)
    {
    }
    return mem;
}
