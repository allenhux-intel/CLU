/*
Copyright (c) 2012, Intel Corporation

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <clu.h>

const int NUM_VALUES = 16;

// example using CLU to write a very simple application to do some OpenCL exploration
int main()
{
    cl_int status = cluInitialize(0);

    char* pK = "kernel void Convert(global float* in_f, global half* out_f) {"
        "    int gid = get_global_id(0);"
        //"out_f[gid] = in_f[gid]; }"; // no free float->half conversion
        "    vstore_half(in_f[gid], gid, out_f);}";

    cl_program p = cluBuildSource(pK, 0, &status);
    if (0 != status)
    {
        const char* err = cluGetBuildErrors(p);
    }
    cl_kernel k = clCreateKernel(p, "Convert", &status);

    // source float values to convert
    cl_float f[NUM_VALUES] = {0};
    for (int i=0;i<NUM_VALUES;i++)
    {
        f[i] = i + 0.3f;
    }

    cl_mem t0 = clCreateBuffer(CLU_CONTEXT, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, NUM_VALUES*sizeof(cl_float), f, &status);
    cl_mem t1 = clCreateBuffer(CLU_CONTEXT, CL_MEM_READ_WRITE, NUM_VALUES*sizeof(cl_half), 0, &status);
  
    // if I had written my kernel in a separate file, I could have used the CLU generator to
    // generate a function that set the kernel arguments for me... but I didn't.
    clSetKernelArg(k, 0, sizeof(cl_mem), &t0);
    clSetKernelArg(k, 1, sizeof(cl_mem), &t1);

    clu_enqueue_params params = CLU_DEFAULT_PARAMS;
    params.nd_range = CLU_ND1(NUM_VALUES);
    status = cluEnqueue(k, &params);

    // inspect the results of converting from 32-bit float to 16-bit float:
    cl_half* pHalf = (cl_half*)clEnqueueMapBuffer(CLU_DEFAULT_Q, t1, CL_TRUE, CL_MAP_READ, 0, NUM_VALUES*sizeof(cl_half), 0, 0, 0, &status);
    status = clEnqueueUnmapMemObject(CLU_DEFAULT_Q, t1, pHalf, 0, 0, 0);

    cluRelease();

    return 0;
}
