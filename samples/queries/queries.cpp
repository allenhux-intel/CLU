/*
Copyright (c) 2013, Intel Corporation

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// example of how to use CLU's query methods:
//   cluGetPlatformInfo
//   cluGetDeviceInfo
//   cluGetSupportedImageFormats

#include <string>
#include <iostream>
#include <sstream>
#include "clu.h"

// conveniently convert int, etc. to std::string
template<typename T> std::string ToString(T x)
{
    std::stringstream i;
    i << x;
    return i.str();
}

// cl_bool isn't sufficiently distinct to use an overloaded ToString<>
#define BoolToStr(in_b) std::string(CL_TRUE==in_b?"true":"false")

void VALIDATE(cl_int in_status)
{
    if (in_status != CL_SUCCESS)
    {
        std::cerr << cluPrintError(in_status) << std::endl;
    }
}

//-----------------------------------------------------------------------------
// print platform info for all platforms
//-----------------------------------------------------------------------------
const char* PrintPlatformInfo()
{
    static std::string s;
    s.clear();

    cl_uint numPlatforms = 0;
    cl_int status = clGetPlatformIDs(0, 0, &numPlatforms);
    VALIDATE(status);

    cl_platform_id* platforms = new cl_platform_id[numPlatforms];
    status = clGetPlatformIDs(numPlatforms, platforms, 0);
    VALIDATE(status);

    s += "-----------------------------------------------------------------------\n";
    s += "\t\tTHIS SYSTEM HAS " + ToString(numPlatforms) + " OPENCL PLATFORM(S) INSTALLED\n";
    s += "-----------------------------------------------------------------------\n\n";

    s += "***********************************************************************\n";
    s += "\t\tBegin Platform Information\n";
    s += "***********************************************************************\n";    
    
    for(unsigned int i=0;i<numPlatforms;i++)
    {
        clu_platform_info platformInfo = cluGetPlatformInfo(platforms[i], 0);

        s += "***********************************************************************\n";
        s += "\t\tDetails for platform " + ToString(i+1) + " of " + ToString(numPlatforms) + ": " + platformInfo.vendor + "\n";
        s += "***********************************************************************\n";
        s += "Platform Name: ";
        s += platformInfo.name;
        s += "\nVendor: ";
        s += platformInfo.vendor;
        s += "\nPlatform Profile: ";
        s += platformInfo.profile;
        s += "\nVersion: ";
        s +=  platformInfo.version;
        s += "\nList of extensions supported:\n ";
        s += platformInfo.extensions;
        s += "\n";
    }

    s += "***********************************************************************\n";
    s += "\t\tEnd Platform Information\n";
    s += "***********************************************************************\n";    
    
    delete [] platforms;
    
    return s.c_str();
}

//-----------------------------------------------------------------------------
// print device info for every device of every platform
//-----------------------------------------------------------------------------
const char* PrintDeviceInfo()
{
    static std::string s;
    s.clear();

    int status;
    cl_uint numPlatforms = 0;

    //first we need to get the number of devices on the platform
    status = clGetPlatformIDs(0, 0, &numPlatforms);
    VALIDATE(status);

    cl_platform_id* platforms = new cl_platform_id[numPlatforms];

    status = clGetPlatformIDs(numPlatforms, platforms, 0);
    VALIDATE(status);

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
        VALIDATE(status);

        cl_device_id* localDevices = new cl_device_id[numDevices];
        status = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, numDevices, localDevices, 0);
        VALIDATE(status);

        for(cl_uint j=0;j<numDevices;j++)
        {
            clu_device_info deviceInfo = cluGetDeviceInfo(localDevices[j], &status);

            s += "***********************************************************************\n";
            s += "\t\tPlatform: " + ToString(i) + ", Device " + ToString(j+1) + " of " + ToString(numDevices) + ", Type: ";

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

            s += "Vendor ID: " + ToString(deviceInfo.vendor_id) + "\n";

            s += "Max Compute Units: " + ToString(deviceInfo.max_compute_units) + "\n";
            s += "Max # of Dimensions: " + ToString(deviceInfo.max_work_item_dims) + "\n";

            s += "Max # items per dim: [" +
                ToString(deviceInfo.max_work_items_per_dimension[0]) + "," +
                ToString(deviceInfo.max_work_items_per_dimension[1]) + "," +
                ToString(deviceInfo.max_work_items_per_dimension[2]) + "]\n";

            s += "Max workgroup size: "  + ToString(deviceInfo.max_wg_size) + "\n";

            s += "\nPreferred Vector Widths:\n"
                "\tchar: "   + ToString(deviceInfo.device_preferred_vector_width_char) +
                ", short: " + ToString(deviceInfo.device_preferred_vector_width_short) +
                ", int: "   + ToString(deviceInfo.device_preferred_vector_width_int) +
                ", long: "  + ToString(deviceInfo.device_preferred_vector_width_long) +
                ", float: " + ToString(deviceInfo.device_preferred_vector_width_float) +
                ", half: "  + ToString(deviceInfo.device_native_vector_width_half) + "\n";

            s += "Native Vector Widths:\n"
                "\tchar: "   + ToString(deviceInfo.device_native_vector_width_char) +
                ", short: " + ToString(deviceInfo.device_native_vector_width_short) +
                ", int: "   + ToString(deviceInfo.device_native_vector_width_int) +
                ", long: "  + ToString(deviceInfo.device_native_vector_width_long) +
                ", float: " + ToString(deviceInfo.device_native_vector_width_float) +
                ", half: "  + ToString(deviceInfo.device_native_vector_width_half) + "\n";

            s += "\nMaximum Clock Freq: " + ToString(deviceInfo.max_clock_frequency) + " Mhz\n";
            s += "Address Bits: " + ToString(deviceInfo.address_bits) + "\n";
            s += "Max Memory Allocation Size: " + ToString(deviceInfo.max_mem_alloc_size) + " bytes\n";

            s += "\nImages";
            s += "\n\tImage Support: " + BoolToStr(deviceInfo.image_support);
            s += "\n\tMax # of simultaneous read images: " + ToString(deviceInfo.max_read_image_args);
            s += "\n\tMax # of simultaneous write images: " + ToString(deviceInfo.max_write_image_args);
            s += "\n\t2D Max [W,H]: [" + ToString(deviceInfo.image2d_max_width) + "," + ToString(deviceInfo.image2d_max_height) + "]";
            s += "\n\t3D Max [W,H,D]:[" + ToString(deviceInfo.image3d_max_width) + "," + ToString(deviceInfo.image3d_max_height) + "," + ToString(deviceInfo.image3d_max_depth) + "]";
            s += "\n\tMax # samplers: " + ToString(deviceInfo.max_samplers) + "\n";

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
            s += "\tMax size of argument that can be passed to kernel: " + ToString(deviceInfo.max_parameter_size) + " bytes\n";
            s += "\tMin. value of largest datatype supported (For alignment): " + ToString(deviceInfo.mem_base_addr_align) + " bits, " + ToString(deviceInfo.min_data_type_align_size) + " bytes\n";
            s += "\tGlobal Memory:\n";
            s += "\t\tGlobal cacheline size: " + ToString(deviceInfo.global_mem_cacheline_size) + " bytes\n";
            s += "\t\tGlobal Cache Size: " + ToString(deviceInfo.global_mem_cache_size) + " bytes\n";
            s += "\t\tGlobal Device Memory: " + ToString(deviceInfo.global_mem_size) + " bytes\n";

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
            s += "\t\tConstant Buffer Size: " + ToString(deviceInfo.constant_buffer_size) + " bytes\n";
            s += "\t\tMax number of constant arguments: " + ToString(deviceInfo.max_constant_args) + "\n";
            s += "\tLocal Memory\n";
            s += "\t\tLocal Memory Size: " + ToString(deviceInfo.local_mem_size) + " bytes\n";
            s += "\t\tError Correction Support: " + ToString(deviceInfo.error_correction_support) + "\n";
            if(deviceInfo.local_mem_type == CL_LOCAL)
            { 
                s += "\t\tHas dedicated local memory storage\n";
            }
            else if(deviceInfo.local_mem_type == CL_GLOBAL)
            {
                s += "\t\tDoes not have Local memory, local memory is actually global\n";
            }

            s += "\tUnified Memory: " + BoolToStr(deviceInfo.unified_memory) + "\n";

            s += "\nProfiling Timer Resolution: " + ToString(deviceInfo.profiling_timer_resolution);
            s += "\nLittle Endian: " + BoolToStr(deviceInfo.endian_little);
            s += "\nDevice Available: " + BoolToStr(deviceInfo.device_available);
            s += "\nCompiler Available: " + BoolToStr(deviceInfo.compiler_available);
            s += "\nPlatform Id: " + ToString(deviceInfo.platform_id) + "\n";

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
                s += "\tCan execute OpenCL kernels out of order\n";
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
            s += "******************** Platform: " + ToString(i) + ", Device " + ToString(j+1) + " End ************************\n";
            s += "***********************************************************************\n\n\n";

        } //end device loop

        delete [] localDevices;
    } //end platform loop


    s += "***********************************************************************\n";
    s += "\t\tEnd Per Platform Device Information\n";
    s += "***********************************************************************\n";    


    delete [] platforms;

    return s.c_str();
}

//-----------------------------------------------------------------------------
// print image format support for every combination of flags
//-----------------------------------------------------------------------------
const char* PrintSupportedImageFormats()
{
    cluInitialize(0);

    static std::string s;
	s.clear();

	s += "*******************************************************************************\n";
	s += "*         Image Formats supported by devices in this context                  *\n"; 
	s += "*  Per device image formats require a single context with a single device     *\n";		
	s += "*******************************************************************************\n";
	
    cl_mem_flags       accessFlags[] = {CL_MEM_READ_WRITE, CL_MEM_WRITE_ONLY, CL_MEM_READ_ONLY};
    cl_mem_flags       ptrFlags[]    = {0, CL_MEM_USE_HOST_PTR, CL_MEM_ALLOC_HOST_PTR, CL_MEM_COPY_HOST_PTR};
    cl_mem_flags       hostFlags[]   = {CL_MEM_HOST_WRITE_ONLY, CL_MEM_HOST_READ_ONLY, CL_MEM_HOST_NO_ACCESS};

    cl_uint numFormats = 0;
    int status;
    const clu_image_format* pImageFormats = cluGetSupportedImageFormats(&numFormats, &status);
    for (cl_uint i = 0; i < numFormats; i++)
    {
        s+= cluPrintChannelOrder(pImageFormats[i].pixelFormat.image_channel_order);
        s += " ";
        s+= cluPrintChannelType(pImageFormats[i].pixelFormat.image_channel_data_type);
        s += " ";
 
        if (CL_TRUE == pImageFormats[i].supports2D) {s+="2D ";}
        if (CL_TRUE == pImageFormats[i].supports2D) {s+="3D ";}
#define MEM_FLAG_TO_STR(F,X) if (F&X) {s+= #X; s+= " ";}
        cl_mem_flags flags = pImageFormats[i].supportedFlags;
        MEM_FLAG_TO_STR(flags, CL_MEM_READ_WRITE);
        MEM_FLAG_TO_STR(flags, CL_MEM_WRITE_ONLY);
        MEM_FLAG_TO_STR(flags, CL_MEM_READ_ONLY);
        MEM_FLAG_TO_STR(flags, CL_MEM_USE_HOST_PTR);
        MEM_FLAG_TO_STR(flags, CL_MEM_ALLOC_HOST_PTR);
        MEM_FLAG_TO_STR(flags, CL_MEM_COPY_HOST_PTR);
        MEM_FLAG_TO_STR(flags, CL_MEM_HOST_WRITE_ONLY);
        MEM_FLAG_TO_STR(flags, CL_MEM_HOST_READ_ONLY);
        MEM_FLAG_TO_STR(flags, CL_MEM_HOST_NO_ACCESS);
        s += "\n";
        s += "\n";
    }

    s += "*******************************************************************************\n";
	s += "****************End Image Formats Supported by the Context*********************\n";
	s += "*******************************************************************************\n";

    cluRelease();

    return s.c_str();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void main()
{
    // use cluGetPlatformInfo to
    // display information about all platforms in the system
    std::cout << PrintPlatformInfo();

    // use cluGetDeviceInfo to
    // display information about all devices in the system (all platforms)
    std::cout << PrintDeviceInfo();

    // use cluGetSupportedImageFormats to
    // display all supported image formats
    std::cout << PrintSupportedImageFormats();
}
