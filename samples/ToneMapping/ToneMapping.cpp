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

//-----------------------------------------------------------------------------
// Execute tone mapping kernel
// this is all the important CL code in this example, from startup to shutdown
//-----------------------------------------------------------------------------
void ExecuteToneMappingKernel(cl_mem inputBuffer, cl_mem outputBuffer, CHDRData *pData, cl_uint imageWidth, cl_uint imageHeight)
{
    cl_int status = CL_SUCCESS;

    clug_ToneMappingPerPixel s = clugCreate_ToneMappingPerPixel(&status);
    CL_VERIFY(status, cluGetBuildErrors(s.m_program));

    // allocate the buffer
    cl_mem HDRDataBuffer = clCreateBuffer(CLU_CONTEXT, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, sizeof(CHDRData), pData, &status);
    CL_VERIFY(status, "Failed to create HDR Data Buffer.\n");

    clu_enqueue_params params = CLU_DEFAULT_PARAMS;
    params.nd_range = cluNDRange2(imageHeight, imageWidth, BLOCK_DIM, BLOCK_DIM, 0, 0);

    status = clugEnqueue_ToneMappingPerPixel(s, &params, inputBuffer, outputBuffer, HDRDataBuffer, imageWidth);

    status = clReleaseMemObject(HDRDataBuffer);

    status = clReleaseKernel(s.m_kernel);
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
bool SaveImageAsBMP (cl_mem in_buffer, int width, int height, const char* fileName)
{
    bool success = false;

    FILE* stream = NULL;
    printf("Save Image: %s \n", fileName);
    stream = fopen( fileName, "wb" );

    if( NULL == stream )
    {
        goto ErrExit;
    }

    cl_uint imageSize = 0;
    clGetMemObjectInfo(in_buffer, CL_MEM_SIZE, sizeof(imageSize), &imageSize, 0);

    cl_float* ptr = (cl_float*)clEnqueueMapBuffer(CLU_DEFAULT_Q,
        in_buffer, CL_TRUE, CL_MAP_READ, 0, imageSize, 0, NULL, NULL, NULL);

    if (0 == ptr)
    {
        goto ErrExit;
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

    success = true;
ErrExit:
    fclose(stream);
    clEnqueueUnmapMemObject(CLU_DEFAULT_Q, in_buffer, ptr, 0, 0, 0);
    return success;
}

//-----------------------------------------------------------------------------
// HDR image loading utility
//-----------------------------------------------------------------------------
cl_mem ReadHDRimage(cl_uint* out_pImageWidth, cl_uint* out_pImageHeight, cl_uint* out_pImageSize)
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
    cl_int status = CL_SUCCESS;
    cl_mem buffer = cluCreateAlignedBuffer(0, iMemSize, 0, &status);

    // map buffer to reach OpenCL synchronization point (for cross-platform compatibility)
    cl_float* pData = (cl_float*)clEnqueueMapBuffer(CLU_DEFAULT_Q,
        buffer, CL_TRUE, CL_MAP_READ, 0, iMemSize, 0, NULL, NULL, &status);

    if(!pData)
    {
        printf("Failed to allocate memory for input HDR image!\n");
        fclose(pRGBAFile);
        return 0;
    }

    // Read data from the input file to memory. 
    fread((void*)pData, 1, iMemSize, pRGBAFile);
    fclose(pRGBAFile);

    // Extended dynamic range 
    for(int i = 0; i < iWidth*iHeight*4; i++)
    {
        pData[i] = 5.0f*pData[i];
    }

    // unmap the host pointer
    status = clEnqueueUnmapMemObject(CLU_DEFAULT_Q, buffer, pData, 0, 0, 0);

    // HDR-image height & width
    *out_pImageWidth = iWidth;
    *out_pImageHeight = iHeight;
    *out_pImageSize = iMemSize;

    return buffer;
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
    cluInitialize(0);

    cl_uint imageWidth;
    cl_uint imageHeight;
    cl_uint imageSize;

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
    
    //read input image
    cl_mem inputBuffer = ReadHDRimage(&imageWidth, &imageHeight, &imageSize);
    if(0 == inputBuffer)
    {
        return -1;
    }

    // create aligned output buffer
    cl_int status = CL_SUCCESS;
    cl_mem outputBuffer = cluCreateAlignedBuffer(0, imageSize, 0, &status);

    printf("Input size is %d X %d\n", imageWidth, imageHeight);

    //do tone mapping
    printf("Executing OpenCL kernel...\n");
    ExecuteToneMappingKernel(inputBuffer, outputBuffer, &HDRData, imageWidth, imageHeight);

    //save results in bitmap files
    SaveImageAsBMP(inputBuffer, imageWidth, imageHeight, "ToneMappingInput.bmp");
    SaveImageAsBMP(outputBuffer, imageWidth, imageHeight, "ToneMappingOutput.bmp");

    clReleaseMemObject(inputBuffer);
    clReleaseMemObject(outputBuffer);

    cluRelease();
    return 0;
}
