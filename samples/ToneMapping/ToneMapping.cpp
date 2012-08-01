/*
Copyright (c) 2012, Intel Corporation

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <math.h>
#include <Windows.h> // for BITMAPFILEHEADER
#include "ToneMapping.cl.h"
#include "chdrdata.h"

#define INPUT_IMAGE "My5_TM.rgb"
#define BLOCK_DIM 16

#define CL_VERIFY(s, m) {if (CL_SUCCESS!=s) {printf("ERROR: %s\n",m);return;}}

cl_int g_min_align = 0;

//-----------------------------------------------------------------------------
// Execute tone mapping kernel
// this is all the important CL code in this example, from startup to shutdown
//-----------------------------------------------------------------------------
void ExecuteToneMappingKernel(cl_float* inputArray, cl_float* outputArray, CHDRData *pData, cl_uint arrayWidth, cl_uint arrayHeight)
{
    cluInitialize(0);

    cl_int status = CL_SUCCESS;

    clug_ToneMappingPerPixel s = clugCreate_ToneMappingPerPixel(&status);
    CL_VERIFY(status, cluGetBuildErrors(s.m_program));

    // allocate the buffer
    cl_mem inputBuffer = clCreateBuffer(CLU_CONTEXT, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, sizeof(cl_float) * 4 * arrayWidth * arrayHeight, inputArray, &status);
    CL_VERIFY(status, "Failed to create Input Buffer.\n");

    cl_mem outputBuffer = clCreateBuffer(CLU_CONTEXT, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR, sizeof(cl_float) * 4 * arrayWidth * arrayHeight, outputArray, &status);
    CL_VERIFY(status, "Failed to create Output Buffer.\n");

    cl_mem HDRDataBuffer = clCreateBuffer(CLU_CONTEXT, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, sizeof(CHDRData), pData, &status);
    CL_VERIFY(status, "Failed to create HDR Data Buffer.\n");

    clu_enqueue_params params = CLU_DEFAULT_PARAMS;
    params.nd_range = cluNDRange2(arrayHeight, arrayWidth, BLOCK_DIM, BLOCK_DIM, 0, 0);

    status = clugEnqueue_ToneMappingPerPixel(s, &params, inputBuffer, outputBuffer, HDRDataBuffer, arrayWidth);
    status = clFinish(CLU_DEFAULT_Q);

    // GPU implementations may require a map to force an update of the host pointer
    void* tmp_ptr = clEnqueueMapBuffer(CLU_DEFAULT_Q, outputBuffer, true, CL_MAP_READ, 0, sizeof(cl_float) *  4 * arrayWidth * arrayHeight , 0, NULL, NULL, NULL);
    if(tmp_ptr!=outputArray)
    {
        printf("ERROR: clEnqueueMapBuffer failed to return original pointer\n");
        return;
    }
    clEnqueueUnmapMemObject(CLU_DEFAULT_Q, outputBuffer, tmp_ptr, 0, NULL, NULL);

    clReleaseMemObject(inputBuffer);
    clReleaseMemObject(outputBuffer);
    clReleaseMemObject(HDRDataBuffer);

    clReleaseKernel(s.m_kernel);
    cluRelease();
}

//-----------------------------------------------------------------------------
// with the right buffer alignment, on some platforms, CL_MEM_USE_HOST_PTR
// will result in the GPU using the host memory without a copy
//-----------------------------------------------------------------------------
void GetMinBufferAlignment()
{
    cluInitialize(0);

    cl_device_id device = cluGetDevice(CL_DEVICE_TYPE_DEFAULT);
    cl_int status = clGetDeviceInfo(device, CL_DEVICE_MEM_BASE_ADDR_ALIGN, sizeof(cl_uint), &g_min_align, NULL);
    CL_VERIFY(status, "Failed to get device information (max memory base address align size).\n");
    g_min_align /= 8; //in bytes

    cluRelease();
}

unsigned char ClampFloat(float in_f)
{
    float f = 255.0f*in_f;
    if (f > 255.0f) f = 255.0f;
    unsigned char c = unsigned char(f);
    return c;
}

//-----------------------------------------------------------------------------
// Write a float RGBA image to a (BGRA) BMP
//-----------------------------------------------------------------------------
bool SaveImageAsBMP (float* ptr, int width, int height, const char* fileName)
{
    FILE* stream = NULL;
    printf("Save Image: %s \n", fileName);
    stream = fopen( fileName, "wb" );

    if( NULL == stream )
    {
        return false;
    }

    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;

    int alignSize  = width * 4;
    alignSize ^= 0x03;
    alignSize ++;
    alignSize &= 0x03;

    int rowLength = width * 4 + alignSize;

    fileHeader.bfReserved1  = 0x0000;
    fileHeader.bfReserved2  = 0x0000;

    infoHeader.biSize          = sizeof(BITMAPINFOHEADER);
    infoHeader.biWidth         = width;
    infoHeader.biHeight        = height;
    infoHeader.biPlanes        = 1;
    infoHeader.biBitCount      = 32;
    infoHeader.biCompression   = BI_RGB;
    infoHeader.biSizeImage     = rowLength * height;
    infoHeader.biXPelsPerMeter = 0;
    infoHeader.biYPelsPerMeter = 0;
    infoHeader.biClrUsed       = 0; // max available
    infoHeader.biClrImportant  = 0;
    fileHeader.bfType       = 0x4D42;
    fileHeader.bfSize       = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + rowLength * height;
    fileHeader.bfOffBits    = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    if(sizeof(BITMAPFILEHEADER) != fwrite( &fileHeader, 1, sizeof(BITMAPFILEHEADER), stream))
    {
        // can't write BITMAPFILEHEADER
        goto ErrExit;
    }

    if(sizeof(BITMAPINFOHEADER) != fwrite( &infoHeader, 1, sizeof(BITMAPINFOHEADER), stream))
    {
        // can't write BITMAPINFOHEADER
        goto ErrExit;
    }

    unsigned char buffer[4] = {0};

    for (int y=0; y<height; y++)
    {
        for (int x=0; x<width; x++)
        {
            int index = (height-1-y)*width+x;
            float* p = &ptr[index*4];
            // Ensure that no value is greater than 255.0
            unsigned char bytes[4] = // RGBA float -> BGRA byte
            {
                ClampFloat(p[2]),
                ClampFloat(p[1]),
                ClampFloat(p[0]),
                ClampFloat(p[3])
            };

            if( 4 != fwrite(bytes, 1, 4, stream))
            {
                goto ErrExit;
            }
        }

        fwrite( buffer, 1, alignSize, stream );
    }

    fclose( stream );
    return true;
ErrExit:
    fclose( stream );
    return false;
}

//-----------------------------------------------------------------------------
// HDR image loading utility
//-----------------------------------------------------------------------------
cl_float* ReadHDRimage(cl_uint* arrayWidth, cl_uint* arrayHeight)
{

    //Load from HDR-image

    //Variables 
    int x = 0;
    int y = 0;
    int iMemSize = 0;
    int iResultMemSize = 0;
    float fTmpVal = 0.0f;
    int iWidth = 0;
    int iHeight = 0;

    FILE* pRGBAFile = fopen(INPUT_IMAGE,"rb");
    if(!pRGBAFile)
    {
        printf("HOST: Failed to open the HDR image file!\n");
        return 0;
    }

    fread((void*)&iWidth, sizeof(int), 1, pRGBAFile);
    fread((void*)&iHeight, sizeof(int), 1, pRGBAFile);
    printf("width = %d\n", iWidth);
    printf("height = %d\n", iHeight);

    if(iWidth<=0 || iHeight<=0 || iWidth > 1000000 || iHeight > 1000000)
    {
        printf("HOST: width or height values are invalid!\n");
        fclose(pRGBAFile);
        return 0;
    }

    // The image size in memory (bytes).
    iMemSize = iWidth*iHeight*4*sizeof(cl_float); 

    // Allocate memory.
    cl_float* inputArray = (cl_float*)_aligned_malloc(iMemSize, g_min_align);
    if(!inputArray)
    {
        printf("Failed to allocate memory for input HDR image!\n");
        fclose(pRGBAFile);
        return 0;
    }

    // Read data from the input file to memory. 
    fread((void*)inputArray, 1, iMemSize, pRGBAFile);

    // Extended dynamic range 
    for(int i = 0; i < iWidth*iHeight*4; i++)
    {
        inputArray[i] = 5.0f*inputArray[i];
    }

    // HDR-image height & width
    *arrayWidth = iWidth;
    *arrayHeight = iHeight;

    fclose(pRGBAFile);

    // Save input image in bitmap file (without tone mapping, just linear scale and crop)
    SaveImageAsBMP( inputArray, iWidth, iHeight, "ToneMappingInput.bmp");

    return inputArray;
}

//-----------------------------------------------------------------------------
// calculate FStops value parameter from the arguments
//-----------------------------------------------------------------------------
float ResetFStopsParameter( float powKLow, float kHigh )
{
    float curveBoxWidth = pow( 2.0f, kHigh ) - powKLow;
    float curveBoxHeight = pow( 2.0f, 3.5f )  - powKLow;

    // Initial boundary values
    float fFStopsLow = 0.0f;
    float fFStopsHigh = 100.0f;
    int iterations = 23; //interval bisection iterations

    // Interval bisection to find the final knee function fStops parameter 
    for ( int i = 0; i < iterations; i++ )
    {
        float fFStopsMiddle = ( fFStopsLow + fFStopsHigh ) * 0.5f;
        if ( ( curveBoxWidth * fFStopsMiddle + 1.0f ) < exp( curveBoxHeight * fFStopsMiddle ) )
        {
            fFStopsHigh = fFStopsMiddle;
        }
        else
        {
            fFStopsLow = fFStopsMiddle;
        }
    }

    return ( fFStopsLow + fFStopsHigh ) * 0.5f;
}

//-----------------------------------------------------------------------------
// main
//-----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    cl_uint arrayWidth;
    cl_uint arrayHeight;

    //init HDR parameters
    float kLow = -3.0f;
    float kHigh = 7.5f;
    float exposure = 3.0f;
    float gamma = 1.0f;
    float defog = 0.0f;

    //fill HDR parameters structure
    CHDRData HDRData;
    HDRData.fGamma = gamma;
    HDRData.fPowGamma = pow(2.0f, -3.5f*gamma);
    HDRData.fDefog = defog;

    HDRData.fPowKLow = pow( 2.0f, kLow );
    HDRData.fPowKHigh = pow( 2.0f, kHigh );
    HDRData.fPow35 = pow(2.0f, 3.5f);
    HDRData.fPowExposure = pow( 2.0f, exposure +  2.47393f );

    //calculate FStops
    HDRData.fFStops = ResetFStopsParameter(HDRData.fPowKLow, kHigh);
    printf("ResetFStopsParameter result = %f\n", HDRData.fFStops);

    HDRData.fFStopsInv = 1.0f/HDRData.fFStops;
    
    //fill input frame
    cl_float* inputArray = 0;

    GetMinBufferAlignment();

    //read input image
    inputArray = ReadHDRimage(&arrayWidth, &arrayHeight);
    if(inputArray==0)
        return -1;


    printf("Input size is %d X %d\n", arrayWidth, arrayHeight);
    cl_float* outputArray = (cl_float*)_aligned_malloc(sizeof(cl_float) * 4 * arrayWidth * arrayHeight, g_min_align);

    //do tone mapping
    printf("Executing OpenCL kernel...\n");
    ExecuteToneMappingKernel(inputArray, outputArray, &HDRData, arrayWidth, arrayHeight);

    //save results in bitmap files
    SaveImageAsBMP(outputArray, arrayWidth, arrayHeight, "ToneMappingOutput.bmp");

    _aligned_free( inputArray );
    _aligned_free( outputArray );

    return 0;
}
