/*
Copyright (c) 2012, Intel Corporation

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// used by clu generator
#define CLU_MAGIC_BUILD_FLAG "CLU_GENERATED_BUILD"

#define CLU_MAXPROPERTYCOUNT 64
#define CLU_MAX_NUM_IMAGE_ENTRIES 256

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
#include <vector>
#include <sstream>
#include <iostream>
#include <malloc.h>

#include <string.h> // gcc needs this for memset
#include "clu.h"

#ifdef _DEBUG
void OCL_VALIDATE(cl_int in_status)
{
    if (in_status != CL_SUCCESS)
    {
		std::cerr
			<< __FILE__ << "(" << __LINE__ << ") OpenCL returned error: "
			<< in_status << " (" << cluPrintError(in_status) << ")" << std::endl;
        // assert(false);
    }
}
#else
#define OCL_VALIDATE(in_status) ((void) in_status)
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
    cl_program   BuildProgram(cl_uint in_numSources,
                              const char** in_sources, const size_t* in_source_lengths,
                              const char* in_buildOptions, cl_int *out_pStatus);
    cl_program   BuildProgram(cl_uint in_numBinaries,
                              const size_t* in_binary_lengths, const unsigned char** in_binaries,
                              const char* in_buildOptions, cl_int *out_pStatus);
    const char*  GetBuildErrors(cl_program program);

    cl_platform_id GetPlatform()                 {return m_platform;}
    cl_device_id GetDevice(cl_device_type in_clDeviceType);
    cl_command_queue GetCommandQueue(cl_device_type in_clDeviceType, cl_int* out_status);
    cl_context   GetContext()                    {return m_context;}
    cl_bool      GetIsInitialized()              {if (m_isInitialized) return CL_TRUE; return CL_FALSE;}
    const char*  GetBuildOptions()               {return m_buildOptions.c_str();}

    // max buffer alignment across all devices in context
    cl_uint      GetBufferAlignment();

    // used by code generator: build and has program on first call,
    // subsequently return hashed program
    cl_program   HashProgram(cl_uint in_numSources,
                              const char** in_sources, const size_t* in_source_lengths,
                              const char* in_buildOptions, cl_int *out_pStatus);
    cl_program   HashProgram(cl_uint in_numBinaries,
                              const size_t* in_binary_lengths, const unsigned char** in_binaries,
                              const char* in_buildOptions, cl_int *out_pStatus);

    // return an array of image formats supported in a given CL context
    const clu_image_format* GetImageFormats(cl_uint* out_pArraySize, cl_int* out_pStatus);

    void Reset(); // set everything to initial state, release all objects
private:
    CLU_Runtime();
    ~CLU_Runtime();

    static CLU_Runtime g_runtime;

    bool             m_isInitialized;
    cl_platform_id   m_platform; // default platform
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
    typedef std::list<CLU_Object*> CluObjectList;
    CluObjectList m_objects;
    // Specific implementations of the object call CL Release methods:
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

    // storage for image format query results from cluGetSupportedImageFormats
    std::vector<clu_image_format> m_imageFormats;

    // build log string from GetBuildErrors
    std::string m_buildString;
};

//-----------------------------------------------------------------------------
// global object
//-----------------------------------------------------------------------------
CLU_Runtime CLU_Runtime::g_runtime;

//-----------------------------------------------------------------------------
// overloaded template definitions for the virtual destructors for
// all the kinds of things that the runtime might hold internal references to:
//-----------------------------------------------------------------------------
template<> CLU_Runtime::CLU_Specific<cl_context>::~CLU_Specific()       {clReleaseContext(m_o);}
template<> CLU_Runtime::CLU_Specific<cl_command_queue>::~CLU_Specific() {clReleaseCommandQueue(m_o);}
template<> CLU_Runtime::CLU_Specific<cl_program>::~CLU_Specific()       {clReleaseProgram(m_o);}

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
    m_platform=0;
    m_context=0;
    m_numDevices=0;
    m_queueProperties = 0;
    m_bufferAlignment = 0;
    m_buildOptions.clear();
    memset(m_commandQueue, 0, sizeof(m_commandQueue));
    memset(m_deviceIds, 0, sizeof(m_deviceIds));

    for (CluObjectList::iterator i = m_objects.begin(); i != m_objects.end(); i++)
    {
        CLU_Object* p = *i;
        delete(p);
    }

    m_objects.clear();
    m_programMap.clear();
    m_imageFormats.clear();
    m_buildString.clear();

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
    cl_platform_id platformId = 0;
    if (i < in_numPlatforms)
    {
        platformId = in_platforms[i];
    }
    return platformId;
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

        if (in_params.vendor_name)
        {
            m_platform = GetPlatformByVendor(numPlatforms, platforms, in_params.vendor_name, &status);
            OCL_VALIDATE(status);
            if (CL_SUCCESS != status) goto exit;
        }

        if (0 == m_platform)
        {
            m_platform = GetPlatformDefault(numPlatforms, platforms, deviceType);
        }

        // should have a platform now.
        if (0 == m_platform)
        {
            status = CL_DEVICE_NOT_FOUND;
            goto exit;
        }

        // get # of devices & device Ids
        // here we have a platform, no context yet
        status = clGetDeviceIDs(m_platform, deviceType, 0, 0, &m_numDevices);
        OCL_VALIDATE(status);
        if (CL_SUCCESS != status) goto exit;

        // get device IDs
        // clamp to CLU_MAX_NUM_DEVICES
        if (m_numDevices > CLU_MAX_NUM_DEVICES)
        {
            m_numDevices = CLU_MAX_NUM_DEVICES;
        }
        status = clGetDeviceIDs(m_platform, deviceType, m_numDevices, m_deviceIds, 0);
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
        properties[propNum] = (cl_context_properties)m_platform;
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
// return command queue based on device type
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
    m_buildString.clear();
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
        m_buildString.append(buildLog);
        delete [] buildLog;
    }
    return m_buildString.c_str();
}

//-----------------------------------------------------------------------------
// Build a program, called by generated code
//-----------------------------------------------------------------------------
cl_program CLU_Runtime::HashProgram(
    cl_uint in_numSources, const char** in_sources, const size_t* in_source_lengths,
    const char* in_buildOptions,
    cl_int*     out_pStatus)
{
    cl_int status = CL_SUCCESS;

    // because this is called by generated code, we can be confident that the
    // source passed in is at an address that is constant for the lifetime of
    // the application. Hence, it's suitable for use as a hash key:
    const void* hashKey = in_sources;

    cl_program program = m_programMap[hashKey]; // find program in hash
    if (0 == program) // hasn't been built yet?
    {
        program = BuildProgram(in_numSources, in_sources, in_source_lengths, in_buildOptions, &status);
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

cl_program CLU_Runtime::HashProgram(
    cl_uint in_numBinaries, const size_t* in_binary_lengths, const unsigned char** in_binaries,
    const char* in_buildOptions,
    cl_int*     out_pStatus)
{
    cl_int status = CL_SUCCESS;

    // because this is called by generated code, we can be confident that the
    // source passed in is at an address that is constant for the lifetime of
    // the application. Hence, it's suitable for use as a hash key:
    const void* hashKey = in_binaries;

    cl_program program = m_programMap[hashKey]; // find program in hash
    if (0 == program) // hasn't been built yet?
    {
        program = BuildProgram(in_numBinaries, in_binary_lengths, in_binaries, in_buildOptions, &status);
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
    cl_uint in_numSources, const char** in_sources, const size_t* in_source_lengths,
    const char* in_buildOptions,
    cl_int*     out_pStatus)
{
    cl_int status = CL_SUCCESS;

    cl_context context = GetContext();
    cl_program program = clCreateProgramWithSource(context, in_numSources, in_sources, in_source_lengths, &status);
    OCL_VALIDATE(status);

    status = clBuildProgram(program, m_numDevices, m_deviceIds, in_buildOptions, 0, 0);
    OCL_VALIDATE(status);

    if (out_pStatus)
    {
        *out_pStatus = status;
    }

    return program;
}

cl_program CLU_Runtime::BuildProgram(
    cl_uint in_numBinaries, const size_t* in_binary_lengths, const unsigned char** in_binaries,
    const char* in_buildOptions,
    cl_int*     out_pStatus)
{
    cl_int status = CL_SUCCESS;

    size_t* pLengths = 0;
    unsigned char **pBinaries = 0;
    if (in_numBinaries < m_numDevices)
    {
        pLengths = new size_t[m_numDevices];
        pBinaries = new unsigned char *[m_numDevices];
        for (cl_uint i = 0; i < m_numDevices; i++)
        {
            pLengths[i] = in_binary_lengths[0];
            pBinaries[i] = (unsigned char*) in_binaries[0];
        }
    }

    cl_context context = GetContext();
    cl_program program = clCreateProgramWithBinary(context, m_numDevices, m_deviceIds, pLengths, (const unsigned char **) pBinaries, 0, &status);
    OCL_VALIDATE(status);

    status = clBuildProgram(program, m_numDevices, m_deviceIds, in_buildOptions, 0, 0);
    OCL_VALIDATE(status);

    if (out_pStatus)
    {
        *out_pStatus = status;
    }

    delete [] pLengths;
    delete [] pBinaries;

    return program;
}

//-----------------------------------------------------------------------------
// find max buffer alignment across all devices in context
//-----------------------------------------------------------------------------
cl_uint CLU_Runtime::GetBufferAlignment()
{
    if (0 == m_bufferAlignment)
    {
        // Some architectures require at least 4KB alignment, in base and size,
        // for 0-copy to work. As of OpenCL 2.0, there is no query for preferred alignment
        // that would reveal this.
        m_bufferAlignment = 4096;

        cl_uint numDevices = 0;
        clGetContextInfo(CLU_CONTEXT, CL_CONTEXT_NUM_DEVICES, sizeof(cl_uint), &numDevices, 0);
        cl_device_id* pDevices = new cl_device_id[numDevices];
        clGetContextInfo(CLU_CONTEXT, CL_CONTEXT_DEVICES, sizeof(cl_device_id)*numDevices, pDevices, 0);
        for (cl_uint i = 0; i < numDevices; i++)
        {
            cl_uint deviceAlign = 0;
            clGetDeviceInfo(pDevices[i], CL_DEVICE_MEM_BASE_ADDR_ALIGN, sizeof(cl_uint), &deviceAlign, 0);
            deviceAlign /= 8; // returns bits! want bytes.
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
// Return cl_platform_id
//   may have been chosen by the runtime, or provided with cluInitialize
//-----------------------------------------------------------------------------
cl_platform_id CLU_API_CALL cluGetPlatform()
{
    return CLU_Runtime::Get().GetPlatform();
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
// Build a program for all current devices from source
//-----------------------------------------------------------------------------
cl_program CLU_API_CALL
cluBuildSource(const char* in_source,
              size_t source_length, /* may be zero */
              cl_int * errcode_ret) /* may be NULL */
{
    cl_program program = 0;
    cl_int status = CL_INVALID_VALUE;

    size_t* pLength = 0;
    if (0 != source_length)
    {
        pLength = &source_length;
    }

    try
    {
        const char* buildOptions = CLU_Runtime::Get().GetBuildOptions();
        program = CLU_Runtime::Get().BuildProgram(1, &in_source, pLength, buildOptions, &status);
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
// Build a program for all current devices from binary
//-----------------------------------------------------------------------------
cl_program CLU_API_CALL
cluBuildBinary(size_t binary_length,
              const unsigned char* in_binary,
              cl_int * errcode_ret) /* may be NULL */
{
    cl_program program = 0;
    cl_int status = CL_INVALID_VALUE;

    size_t* pLength = 0;
    if (0 != binary_length)
    {
        pLength = &binary_length;
    }

    try
    {
        const char* buildOptions = CLU_Runtime::Get().GetBuildOptions();
        program = CLU_Runtime::Get().BuildProgram(1, pLength, &in_binary, buildOptions, &status);
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
// Build a program for all current devices from source
//   can override global build options
//-----------------------------------------------------------------------------
cl_program CLU_API_CALL
cluBuildSourceArray(cl_uint num_sources,
                   const char** sources,
                   const size_t* source_lengths, /* may be NULL */
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
            program = CLU_Runtime::Get().HashProgram(num_sources, sources, source_lengths,
                compile_options, &status);
        }
        else
        {
            if (0 == compile_options)
            {
                compile_options = CLU_Runtime::Get().GetBuildOptions();
            }
            program = CLU_Runtime::Get().BuildProgram(num_sources, sources, source_lengths,
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
// Build a program for all current devices from binary
//   can override global build options
//-----------------------------------------------------------------------------
cl_program CLU_API_CALL
cluBuildBinaryArray(cl_uint num_binaries,
                   const size_t* binary_lengths,
                   const unsigned char** binaries,
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
            program = CLU_Runtime::Get().HashProgram(num_binaries, binary_lengths, binaries,
                compile_options, &status);
        }
        else
        {
            if (0 == compile_options)
            {
                compile_options = CLU_Runtime::Get().GetBuildOptions();
            }
            program = CLU_Runtime::Get().BuildProgram(num_binaries, binary_lengths, binaries,
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
            std::ifstream ifs(in_pFileName, std::ios::in);
            if (ifs.is_open())
            {
                std::string s((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
                program = cluBuildSource((const char *) s.c_str(), s.size(), &status);
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

//-----------------------------------------------------------------------------
// convert a binary file into a cl_program
//-----------------------------------------------------------------------------
cl_program CLU_API_CALL
cluBuildBinaryFromFile(const char* in_pFileName, cl_int * errcode_ret)
{
    cl_int status = CL_INVALID_VALUE;
    cl_program program = 0;
    try
    {
        if (in_pFileName)
        {
            std::ifstream ifs(in_pFileName, std::ios::in | std::ios::binary);
            if (ifs.is_open())
            {
                std::string s((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
                program = cluBuildBinary(s.size(), (const unsigned char *) s.c_str(), &status);
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

//-----------------------------------------------------------------------------
// release a buffer and its aligned host memory
// (expects created with cluCreateAlignedBuffer)
//-----------------------------------------------------------------------------
void CL_CALLBACK CLU_ReleaseAlignedBufferCallback(cl_mem in_buffer, void* in_pHostPtr)
{
    in_buffer = 0; // unused, fixes compile warning about unused param.
#if defined _WIN32            
    _aligned_free(in_pHostPtr);
#else
    free(in_pHostPtr);
#endif
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
        cl_int status = CL_INVALID_VALUE;
        void* pMem = 0;

        // create aligned host memory
        if (in_size)
        {
            cl_uint alignment = CLU_Runtime::Get().GetBufferAlignment();

            // round size up to the alignment.
            // some architectures require aligned size for 0-copy to work properly.
            in_size += (alignment - 1);
            in_size &= ~(alignment - 1);

#if defined _WIN32
            pMem = _aligned_malloc(in_size, alignment);
#elif defined __linux__
            pMem = memalign(alignment, in_size);
#elif defined __MACH__
            pMem = malloc(in_size);
#else
            pMem = valloc(in_size);
#endif
        }

        // wrap aligned host memory in an OpenCL buffer
        if (pMem)
        {
            mem = clCreateBuffer(CLU_CONTEXT, in_flags | CL_MEM_USE_HOST_PTR, in_size, pMem, out_pStatus);
        }
        else
        {
            status = CL_INVALID_VALUE;
        }

        // set a callback to automatically free the host memory when the OpenCL buffer is destroyed
        if (mem)
        {
            status = clSetMemObjectDestructorCallback(mem, CLU_ReleaseAlignedBufferCallback, pMem);
            if (out_pPtr)
            {
                *out_pPtr = pMem;
            }
        }
        else
        {
            status = CL_OUT_OF_HOST_MEMORY;
#if defined _WIN32            
            _aligned_free(out_pPtr);
#else
            free(out_pPtr);
#endif
        }
        if (out_pStatus)
        {
            *out_pStatus = status;
        }
    }
    catch (...)
    {
    }
    return mem;
}

//-----------------------------------------------------------------------------
// wait until /any/ event in the list fires
//-----------------------------------------------------------------------------
void CL_CALLBACK CLU_WaitOnAnyEventCallback(cl_event in_event, cl_int in_eventStatus, void* in_data)
{
    in_event = 0; // unused, remove compiler warning
    assert(CL_SUCCESS == in_eventStatus);
    clSetUserEventStatus((cl_event)in_data, CL_COMPLETE);
    clReleaseEvent((cl_event)in_data);
}

// wait on any event implementation:
//   1. create a user event
//   2. set a callback on every event in the input list, this callback sets the user event
//   3. wait on the internal user event
cl_int CLU_API_CALL
cluWaitOnAnyEvent(const cl_event* event_list,
                  cl_uint         num_events)
{
    cl_int status = CL_INVALID_EVENT_WAIT_LIST;

    // need an early exit, or 0-length list results in infinite wait
    if ((0 == event_list) || (0 == num_events))
    {
        goto exit;
    }

    try
    {
        cl_event waitEvent = clCreateUserEvent(cluGetContext(), &status);
        if (CL_SUCCESS == status)
        {
            for (cl_uint i = 0; i < num_events; i++)
            {
                status = clRetainEvent(waitEvent); // the callback will use the event, so hold a reference
                status = clSetEventCallback(event_list[i], CL_COMPLETE, CLU_WaitOnAnyEventCallback, waitEvent);
                if (CL_SUCCESS != status)
                {
                    break; // failed for some reason
                }
            }
        } // end if got user event for internal callback function

        if (CL_SUCCESS == status)
        {
            status = clWaitForEvents(1, &waitEvent);
        }
        clReleaseEvent(waitEvent);
    }
    catch (...)
    {
    }
exit:
    return status;
}

//=============================================================================
//utility functions and structs
//=============================================================================

//-----------------------------------------------------------------------------
// utility to populate platform information struct
//-----------------------------------------------------------------------------
clu_platform_info CLU_API_CALL cluGetPlatformInfo(cl_platform_id in_platformId, cl_int* out_pStatus)
{
    clu_platform_info platformInfo = {0};
    cl_int status = CL_OUT_OF_HOST_MEMORY;
    try
    {
        status = clGetPlatformInfo(in_platformId, CL_PLATFORM_PROFILE,    CLU_UTIL_MAX_STRING_LENGTH, platformInfo.profile, 0);
        status = clGetPlatformInfo(in_platformId, CL_PLATFORM_VERSION,    CLU_UTIL_MAX_STRING_LENGTH, platformInfo.version, 0);
        status = clGetPlatformInfo(in_platformId, CL_PLATFORM_NAME,       CLU_UTIL_MAX_STRING_LENGTH, platformInfo.name,    0);
        status = clGetPlatformInfo(in_platformId, CL_PLATFORM_VENDOR,     CLU_UTIL_MAX_STRING_LENGTH, platformInfo.vendor,  0);
        status = clGetPlatformInfo(in_platformId, CL_PLATFORM_EXTENSIONS, CLU_UTIL_PLATFORM_EXTENSION_ARRAY_SIZE, platformInfo.extensions, 0);
    }
    catch (...) // internal error
    {
    }
    if (out_pStatus)
    {
        *out_pStatus = status;
    }
    return platformInfo;
}

//-----------------------------------------------------------------------------
// utility to acquire device information string for all devices in all platforms.
//-----------------------------------------------------------------------------
clu_device_info CLU_API_CALL cluGetDeviceInfo(cl_device_id in_deviceId, cl_int* out_pStatus)
{
    clu_device_info deviceInfo = {0};
    cl_int status = CL_INVALID_OPERATION;
    try
    {
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_TYPE, sizeof(cl_device_type), &deviceInfo.devType, 0);

        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_VENDOR_ID, sizeof(cl_device_type), &deviceInfo.vendor_id, 0);

        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &deviceInfo.max_compute_units, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(cl_uint), &deviceInfo.max_work_item_dims, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_MAX_WORK_ITEM_SIZES, (sizeof(size_t)*3), &deviceInfo.max_work_items_per_dimension, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(cl_uint), &deviceInfo.max_wg_size, 0);

        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR, sizeof(size_t), &deviceInfo.device_preferred_vector_width_char, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT, sizeof(cl_uint), &deviceInfo.device_preferred_vector_width_short, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT, sizeof(cl_uint), &deviceInfo.device_preferred_vector_width_int, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG, sizeof(cl_uint), &deviceInfo.device_preferred_vector_width_long, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT, sizeof(cl_uint), &deviceInfo.device_preferred_vector_width_float, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF, sizeof(cl_uint), &deviceInfo.device_preferred_vector_width_half, 0);

        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR, sizeof(size_t), &deviceInfo.device_native_vector_width_char, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT, sizeof(cl_uint), &deviceInfo.device_native_vector_width_short, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_NATIVE_VECTOR_WIDTH_INT, sizeof(cl_uint), &deviceInfo.device_native_vector_width_int, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG, sizeof(cl_uint), &deviceInfo.device_native_vector_width_long, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT, sizeof(cl_uint), &deviceInfo.device_native_vector_width_float, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF, sizeof(cl_uint), &deviceInfo.device_native_vector_width_half, 0);

        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(cl_uint), &deviceInfo.max_clock_frequency, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_ADDRESS_BITS, sizeof(cl_uint), &deviceInfo.address_bits, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(cl_ulong), &deviceInfo.max_mem_alloc_size, 0);

        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_IMAGE_SUPPORT, sizeof(cl_bool), &deviceInfo.image_support, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_MAX_READ_IMAGE_ARGS, sizeof(cl_uint), &deviceInfo.max_read_image_args, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_MAX_WRITE_IMAGE_ARGS, sizeof(cl_uint), &deviceInfo.max_write_image_args, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_IMAGE2D_MAX_WIDTH, sizeof(size_t), &deviceInfo.image2d_max_width, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_IMAGE2D_MAX_HEIGHT, sizeof(size_t), &deviceInfo.image2d_max_height, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_IMAGE3D_MAX_WIDTH, sizeof(size_t), &deviceInfo.image3d_max_width, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_IMAGE3D_MAX_HEIGHT, sizeof(size_t), &deviceInfo.image3d_max_height, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_IMAGE3D_MAX_DEPTH, sizeof(size_t), &deviceInfo.image3d_max_depth, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_MAX_SAMPLERS, sizeof(cl_uint), &deviceInfo.max_samplers, 0);

        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_MAX_PARAMETER_SIZE, sizeof(size_t), &deviceInfo.max_parameter_size, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_MEM_BASE_ADDR_ALIGN, sizeof(cl_uint), &deviceInfo.mem_base_addr_align, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE, sizeof(cl_uint), &deviceInfo.min_data_type_align_size, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_SINGLE_FP_CONFIG, sizeof(cl_device_fp_config), &deviceInfo.single_fp_config, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_GLOBAL_MEM_CACHE_TYPE, sizeof(cl_device_mem_cache_type), &deviceInfo.global_mem_cache_type, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE, sizeof(cl_uint), &deviceInfo.global_mem_cacheline_size, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, sizeof(cl_ulong), &deviceInfo.global_mem_cache_size, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(cl_ulong), &deviceInfo.global_mem_size, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE, sizeof(cl_ulong), &deviceInfo.constant_buffer_size, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_MAX_CONSTANT_ARGS, sizeof(cl_uint), &deviceInfo.max_constant_args, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_LOCAL_MEM_TYPE, sizeof(cl_device_local_mem_type), &deviceInfo.local_mem_type, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), &deviceInfo.local_mem_size, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_ERROR_CORRECTION_SUPPORT, sizeof(cl_bool), &deviceInfo.error_correction_support, 0);
            
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_HOST_UNIFIED_MEMORY, sizeof(cl_bool), &deviceInfo.unified_memory, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_PROFILING_TIMER_RESOLUTION, sizeof(size_t), &deviceInfo.profiling_timer_resolution, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_ENDIAN_LITTLE, sizeof(cl_bool), &deviceInfo.endian_little, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_AVAILABLE, sizeof(cl_bool), &deviceInfo.device_available, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_COMPILER_AVAILABLE, sizeof(cl_bool), &deviceInfo.compiler_available, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_EXECUTION_CAPABILITIES, sizeof(cl_device_exec_capabilities), &deviceInfo.device_capabilities, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_QUEUE_PROPERTIES, sizeof(cl_command_queue_properties), &deviceInfo.queue_properties, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_PLATFORM, sizeof(cl_platform_id), &deviceInfo.platform_id, 0);
    
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_NAME,             CLU_UTIL_MAX_STRING_LENGTH, &deviceInfo.device_name, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_VENDOR,           CLU_UTIL_MAX_STRING_LENGTH, &deviceInfo.device_vendor, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DRIVER_VERSION,          CLU_UTIL_MAX_STRING_LENGTH, &deviceInfo.driver_version, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_PROFILE,          CLU_UTIL_MAX_STRING_LENGTH, &deviceInfo.device_profile, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_VERSION,          CLU_UTIL_MAX_STRING_LENGTH, &deviceInfo.device_version, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_OPENCL_C_VERSION, CLU_UTIL_MAX_STRING_LENGTH, &deviceInfo.opencl_c_version, 0);
        status = clGetDeviceInfo(in_deviceId, CL_DEVICE_EXTENSIONS,       CLU_UTIL_PLATFORM_EXTENSION_ARRAY_SIZE, &deviceInfo.extensions, 0);
    }
    catch (...) // internal error
    {
    }
    if (out_pStatus)
    {
        *out_pStatus = status;
    }
    return deviceInfo;
}

//-----------------------------------------------------------------------------
// Utility: return an array of image formats supported in a given CL context
// the array returned is internal to CLU, applications should not attempt to free/delete it
//-----------------------------------------------------------------------------
const clu_image_format* CLU_Runtime::GetImageFormats(cl_uint* out_pArraySize, cl_int* out_pStatus)
{
    m_imageFormats.resize(0);

    cl_mem_object_type imageTypes[]  = {CL_MEM_OBJECT_IMAGE2D, CL_MEM_OBJECT_IMAGE3D};
    cl_mem_flags       accessFlags[] = {CL_MEM_READ_WRITE, CL_MEM_WRITE_ONLY, CL_MEM_READ_ONLY};
    cl_mem_flags       ptrFlags[]    = {0, CL_MEM_USE_HOST_PTR, CL_MEM_ALLOC_HOST_PTR, CL_MEM_COPY_HOST_PTR};
    cl_mem_flags       hostFlags[]   = {CL_MEM_HOST_WRITE_ONLY, CL_MEM_HOST_READ_ONLY, CL_MEM_HOST_NO_ACCESS};

    struct FormatLess
    {
        bool operator()(cl_image_format const& left, cl_image_format const& right) const
        {
            bool less = (
                (left.image_channel_order < right.image_channel_order) ||
                ((left.image_channel_order == right.image_channel_order) &&
                (left.image_channel_data_type < right.image_channel_data_type))
                );
            return less;
        }
    };
    typedef std::map<cl_image_format, cl_uint, FormatLess> FormatMap;
    FormatMap formatMap;
    cl_context ctx = cluGetContext();
    cl_image_format* formats = 0;
    cl_uint numFormats = 0;

    // loop over all image dimensions (2D, 3D)
    for (int i = 0; i < 2; i++)
    {
        cl_mem_object_type imageType = imageTypes[i];

        // loop over data access flags (read only, write only, read+write)
        for (int a = 0; a < 3; a++)
        {
            cl_mem_flags access = accessFlags[a];

            // loop over host pointer support
            // note that "0" (no corresponding host pointer) is supported by all types
            for (int p = 0; p < 3; p++)
            {
                cl_mem_flags ptr = ptrFlags[p];

                // loop over host access flags: can the host read or write to the image
                for (int h = 0; h < 3; h++)
                {
                    cl_mem_flags host = hostFlags[h];

                    cl_mem_flags memFlags = access | ptr | host;

                    int status;
                    cl_uint numSupportedFormats;

                    // query context for all the supported image formats with the requested flags
                    status = clGetSupportedImageFormats(ctx, memFlags, imageType, 0, 0, &numSupportedFormats);

                    if (numSupportedFormats > numFormats)
                    {
                        delete [] formats;
                        numFormats = numSupportedFormats;
                        formats = new cl_image_format[numFormats];
                    }

                    // query context for all the supported image formats with the requested flags
                    status = clGetSupportedImageFormats(ctx, memFlags, imageType, numFormats, formats, 0);

                    // now, update CLU's flags for each image type
                    // find each of the formats in the map
                    // if not there, add it to the map
                    // then, OR in the flags we just searched for
                    for (cl_uint f = 0; f < numFormats; f++)
                    {
                        FormatMap::const_iterator iter = formatMap.find(formats[f]);
                        cl_uint index = 0;

                        // have we seen this format before?
                        bool newFormat = (iter == formatMap.end());
                        if (newFormat)
                        {
                            index = (cl_uint)m_imageFormats.size();
                            m_imageFormats.resize(index+1);
                            formatMap[formats[f]] = index;
                            m_imageFormats[index].pixelFormat = formats[f];
                            m_imageFormats[index].supportedFlags = 0;
                            m_imageFormats[index].supports2D = CL_FALSE;
                            m_imageFormats[index].supports3D = CL_FALSE;
                        }
                        else
                        {
                            index = iter->second;
                            assert(index < CLU_MAX_NUM_IMAGE_ENTRIES);
                        }

                        if (CL_MEM_OBJECT_IMAGE2D == imageType) {m_imageFormats[index].supports2D = CL_TRUE;}
                        else {m_imageFormats[index].supports3D = CL_TRUE;}

                        m_imageFormats[index].supportedFlags |= memFlags;
                    }

                    if (out_pStatus)
                    {
                        *out_pStatus = status;
                    }
                } // end loop over host access flags
            } // end loop over host pointer type flags
        } // end loop over access flags
    } // end loop over dimensions (2d/3d)

    delete [] formats;

    *out_pArraySize = (cl_uint)m_imageFormats.size();
    return &m_imageFormats[0];
}

//-----------------------------------------------------------------------------
// Return an array of image formats supported in a given CL context
// the array returned is internal to CLU, applications should not attempt to free/delete it
//-----------------------------------------------------------------------------
const clu_image_format* CLU_API_CALL cluGetSupportedImageFormats(cl_uint* array_size, cl_int* out_pStatus)
{
	//FIXME: consider using the query to first get the number of format descriptors that get returned then allocate clu_image_formats rightsized
    cl_int status;
    const clu_image_format* pFormats = 0;
    try
    {
        pFormats = CLU_Runtime::Get().GetImageFormats(array_size, &status);
    }
    catch (...) // internal error, e.g. thrown by STL
    {
        status = CL_OUT_OF_HOST_MEMORY;
    }
    if (out_pStatus)
    {
        *out_pStatus = status;
    }
    return pFormats;
}

/********************************************************************************************************/
/* String Functions: convert enums/defines to char*                                                     */
/********************************************************************************************************/

#define CLU_ENUM_TO_STRING_CASE(X) case X : return #X;

//-----------------------------------------------------------------------------
// Return a string from an OpenCL return code, e.g. CL_SUCCESS returns "CL_SUCCESS"
//-----------------------------------------------------------------------------
const char* CLU_API_CALL cluPrintError(cl_int errcode_ret)
{
    switch (errcode_ret)
    {
        CLU_ENUM_TO_STRING_CASE(CL_SUCCESS);
        CLU_ENUM_TO_STRING_CASE(CL_DEVICE_NOT_FOUND);
        CLU_ENUM_TO_STRING_CASE(CL_DEVICE_NOT_AVAILABLE);
        CLU_ENUM_TO_STRING_CASE(CL_COMPILER_NOT_AVAILABLE);
        CLU_ENUM_TO_STRING_CASE(CL_MEM_OBJECT_ALLOCATION_FAILURE);
        CLU_ENUM_TO_STRING_CASE(CL_OUT_OF_RESOURCES);
        CLU_ENUM_TO_STRING_CASE(CL_OUT_OF_HOST_MEMORY);
        CLU_ENUM_TO_STRING_CASE(CL_PROFILING_INFO_NOT_AVAILABLE);
        CLU_ENUM_TO_STRING_CASE(CL_MEM_COPY_OVERLAP);
        CLU_ENUM_TO_STRING_CASE(CL_IMAGE_FORMAT_MISMATCH);
        CLU_ENUM_TO_STRING_CASE(CL_IMAGE_FORMAT_NOT_SUPPORTED);
        CLU_ENUM_TO_STRING_CASE(CL_BUILD_PROGRAM_FAILURE);
        CLU_ENUM_TO_STRING_CASE(CL_MAP_FAILURE);
        CLU_ENUM_TO_STRING_CASE(CL_INVALID_VALUE);
        CLU_ENUM_TO_STRING_CASE(CL_INVALID_DEVICE_TYPE);
        CLU_ENUM_TO_STRING_CASE(CL_INVALID_PLATFORM);
        CLU_ENUM_TO_STRING_CASE(CL_INVALID_DEVICE);
        CLU_ENUM_TO_STRING_CASE(CL_INVALID_CONTEXT);
        CLU_ENUM_TO_STRING_CASE(CL_INVALID_QUEUE_PROPERTIES);
        CLU_ENUM_TO_STRING_CASE(CL_INVALID_COMMAND_QUEUE);
        CLU_ENUM_TO_STRING_CASE(CL_INVALID_HOST_PTR);
        CLU_ENUM_TO_STRING_CASE(CL_INVALID_MEM_OBJECT);
        CLU_ENUM_TO_STRING_CASE(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR);
        CLU_ENUM_TO_STRING_CASE(CL_INVALID_IMAGE_SIZE);
        CLU_ENUM_TO_STRING_CASE(CL_INVALID_SAMPLER);
        CLU_ENUM_TO_STRING_CASE(CL_INVALID_BINARY);
        CLU_ENUM_TO_STRING_CASE(CL_INVALID_BUILD_OPTIONS);
        CLU_ENUM_TO_STRING_CASE(CL_INVALID_PROGRAM);
        CLU_ENUM_TO_STRING_CASE(CL_INVALID_PROGRAM_EXECUTABLE);
        CLU_ENUM_TO_STRING_CASE(CL_INVALID_KERNEL_NAME);
        CLU_ENUM_TO_STRING_CASE(CL_INVALID_KERNEL_DEFINITION);
        CLU_ENUM_TO_STRING_CASE(CL_INVALID_KERNEL);
        CLU_ENUM_TO_STRING_CASE(CL_INVALID_ARG_INDEX);
        CLU_ENUM_TO_STRING_CASE(CL_INVALID_ARG_VALUE);
        CLU_ENUM_TO_STRING_CASE(CL_INVALID_ARG_SIZE);
        CLU_ENUM_TO_STRING_CASE(CL_INVALID_KERNEL_ARGS);
        CLU_ENUM_TO_STRING_CASE(CL_INVALID_WORK_DIMENSION);
        CLU_ENUM_TO_STRING_CASE(CL_INVALID_WORK_GROUP_SIZE);
        CLU_ENUM_TO_STRING_CASE(CL_INVALID_WORK_ITEM_SIZE);
        CLU_ENUM_TO_STRING_CASE(CL_INVALID_GLOBAL_OFFSET);
        CLU_ENUM_TO_STRING_CASE(CL_INVALID_EVENT_WAIT_LIST);
        CLU_ENUM_TO_STRING_CASE(CL_INVALID_EVENT);
        CLU_ENUM_TO_STRING_CASE(CL_INVALID_OPERATION);
        CLU_ENUM_TO_STRING_CASE(CL_INVALID_GL_OBJECT);
        CLU_ENUM_TO_STRING_CASE(CL_INVALID_BUFFER_SIZE);
        CLU_ENUM_TO_STRING_CASE(CL_INVALID_MIP_LEVEL);
        CLU_ENUM_TO_STRING_CASE(CL_INVALID_GLOBAL_WORK_SIZE);
#ifdef CL_VERSION_1_2
        CLU_ENUM_TO_STRING_CASE(CL_INVALID_PROPERTY);
        CLU_ENUM_TO_STRING_CASE(CL_INVALID_IMAGE_DESCRIPTOR);
        CLU_ENUM_TO_STRING_CASE(CL_INVALID_COMPILER_OPTIONS);
        CLU_ENUM_TO_STRING_CASE(CL_INVALID_LINKER_OPTIONS);
        CLU_ENUM_TO_STRING_CASE(CL_INVALID_DEVICE_PARTITION_COUNT);
#endif
    }
    return "Unknown CL error";
}

//-----------------------------------------------------------------------------
// Return a string from image channel type
//-----------------------------------------------------------------------------
const char* CLU_API_CALL cluPrintChannelOrder(cl_channel_order in_channelOrder)
{
	switch(in_channelOrder)
	{
        CLU_ENUM_TO_STRING_CASE(CL_R);
        CLU_ENUM_TO_STRING_CASE(CL_A);
        CLU_ENUM_TO_STRING_CASE(CL_RG);
        CLU_ENUM_TO_STRING_CASE(CL_RA);
        CLU_ENUM_TO_STRING_CASE(CL_RGB);
        CLU_ENUM_TO_STRING_CASE(CL_RGBA);
        CLU_ENUM_TO_STRING_CASE(CL_BGRA);
        CLU_ENUM_TO_STRING_CASE(CL_ARGB);
        CLU_ENUM_TO_STRING_CASE(CL_INTENSITY);
        CLU_ENUM_TO_STRING_CASE(CL_LUMINANCE);
        CLU_ENUM_TO_STRING_CASE(CL_Rx);
        CLU_ENUM_TO_STRING_CASE(CL_RGx);
        CLU_ENUM_TO_STRING_CASE(CL_RGBx);

        // note: some ocl implementations ship a version 1.2 header that is incomplete
        // hence, can't just check version number
#ifdef CL_DEPTH
        CLU_ENUM_TO_STRING_CASE(CL_DEPTH);
        CLU_ENUM_TO_STRING_CASE(CL_DEPTH_STENCIL);
#endif
	default:
		return "Unknown";
		break;
	}
}

//-----------------------------------------------------------------------------
// Return a string from image channel order
//-----------------------------------------------------------------------------
const char* CLU_API_CALL cluPrintChannelType(cl_channel_type in_channelType)
{
	switch(in_channelType)
	{
        CLU_ENUM_TO_STRING_CASE(CL_SNORM_INT8);
        CLU_ENUM_TO_STRING_CASE(CL_SNORM_INT16);
        CLU_ENUM_TO_STRING_CASE(CL_UNORM_INT8);
        CLU_ENUM_TO_STRING_CASE(CL_UNORM_INT16);
        CLU_ENUM_TO_STRING_CASE(CL_UNORM_SHORT_565);
        CLU_ENUM_TO_STRING_CASE(CL_UNORM_SHORT_555);
        CLU_ENUM_TO_STRING_CASE(CL_UNORM_INT_101010);
        CLU_ENUM_TO_STRING_CASE(CL_SIGNED_INT8);
        CLU_ENUM_TO_STRING_CASE(CL_SIGNED_INT16);
        CLU_ENUM_TO_STRING_CASE(CL_SIGNED_INT32);
        CLU_ENUM_TO_STRING_CASE(CL_UNSIGNED_INT8);
        CLU_ENUM_TO_STRING_CASE(CL_UNSIGNED_INT16);
        CLU_ENUM_TO_STRING_CASE(CL_UNSIGNED_INT32);
        CLU_ENUM_TO_STRING_CASE(CL_HALF_FLOAT);
        CLU_ENUM_TO_STRING_CASE(CL_FLOAT);

        // note: some ocl implementations ship a version 1.2 header that is incomplete
        // hence, can't just check version number
#ifdef CL_UNORM_INT24
        CLU_ENUM_TO_STRING_CASE(CL_UNORM_INT24);
#endif
	default:
		return "Unknown";
		break;
	}
}

