/*
Copyright (c) 2012, Intel Corporation

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include "simple.cl.h"

void main()
{
    int* mapped = 0;
    clu_enqueue_params params;

    // initialize CLU
    cl_int status = cluInitialize(0);

    // create my buffer
    int bufferLength = 1024;
    int bufferSize = sizeof(int)*bufferLength;
    cl_mem buf = clCreateBuffer(CLU_CONTEXT, CL_MEM_WRITE_ONLY, bufferSize, 0, &status);

    // get a function that will enqueue my kernel
    clug_Simple s = clugCreate_Simple(&status);
    if (CL_SUCCESS != status)
    {
        printf("%s\n",cluGetBuildErrors(s.m_program));
    }

    // enqueue my kernel
    params = CLU_DEFAULT_PARAMS;
    params.nd_range = CLU_ND1(bufferLength);
    status = clugEnqueue_Simple(s, &params, 42, buf);

    // check the results
    mapped = (int*)clEnqueueMapBuffer(CLU_DEFAULT_Q, buf, CL_TRUE, CL_MAP_READ, 0, bufferSize, 0, 0, 0, &status);
    clEnqueueUnmapMemObject(CLU_DEFAULT_Q, buf, mapped, 0, 0, 0);

    clReleaseMemObject(buf);

    // done using CLU
    cluRelease();
}
