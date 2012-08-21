/*
Copyright (c) 2012, Intel Corporation

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS

#include <stdio.h>
#include "SimulateCL.h"
#include "math/mathlib.h"
#include <time.h>

// GPU will use clFlush for GL/CL sync. CPU will use slower clFinish.
#define SIM_DEVICE_TYPE CL_DEVICE_TYPE_GPU

//-----------------------------------------------------------------------------
// Initialize system, booting up CL
//-----------------------------------------------------------------------------
cl_int SimulateCL::Init(void* in_hGLRC, void* in_hDC,
        int in_numPoints,
        cl_GLuint in_points,
        float in_range, // size of space
        float in_speed) // maximum speed of particles
{
    cl_context_properties properties[] =
    { 
        CL_GL_CONTEXT_KHR, (cl_context_properties)in_hGLRC,
        CL_WGL_HDC_KHR,    (cl_context_properties)in_hDC,
        0 
    };

    clu_initialize_params params = {0};
    //params.vendor_name = "Intel";
    params.preferred_device_type = SIM_DEVICE_TYPE;
    params.default_context_props = properties;

    //-----------
    // start CLU
    //-----------
    cl_int status = cluInitialize(&params);

    m_cluKernel = clugCreate_Simulate(&status);
    if (CL_SUCCESS != status)
    {
        const char* errors = cluGetBuildErrors(m_cluKernel.m_program);
        printf("%s\n", errors);
    }

    ShareBuffers(in_numPoints, in_points, in_range, in_speed);
    return status;
}

//-----------------------------------------------------------------------------
// return float in range -1..1
//-----------------------------------------------------------------------------
float FloatRand()
{
    //float f = float(rand() + rand());
    float f = 2.0f * rand();
    f /= RAND_MAX; // f = 0..2
    f -= 1.0f;     // f = -1..1
    return f;
}

//-----------------------------------------------------------------------------
// share DX buffers
//-----------------------------------------------------------------------------
void SimulateCL::ShareBuffers(
    int in_numPoints,
    cl_GLuint in_points,
    float in_range, // size of space
    float in_speed) // maximum speed of particles
{
    srand(clock());

    m_numPoints = in_numPoints;
    m_range = in_range;

    cl_int status;
    m_points = clCreateFromGLBuffer(CLU_CONTEXT, CL_MEM_READ_WRITE, in_points, &status);

    Vector4* points = new Vector4[m_numPoints];
    Vector4* velocities = new Vector4[m_numPoints];

    for (int i = 0; i < m_numPoints; i++)
    {
        // this seems to provide a nicer distribution than polar coordinates:
        Vector3 pos = in_range * Vector3(FloatRand(), FloatRand(), FloatRand());
        // clamp to distance
        float distSquared = pos.MagnitudeSq();
        if (distSquared > (in_range*in_range))
        {
            pos = in_range * pos.Normalize();
        }
        Vector3 vel = in_speed * Vector3(FloatRand(), FloatRand(), FloatRand());
        points[i] = Vector4(pos,0);
        velocities[i] = Vector4(vel,0);
    }

    int pointsSize = m_numPoints * sizeof(Vector4);

    // The buffer was created using GL and may not be writeable by
    // the CPU (e.g. created for a discrete GPU device).
    // So, to initialize, use clCopyBuffer instead of map/memcpy/unmap

    cl_mem temp = clCreateBuffer(CLU_CONTEXT, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, pointsSize, points, &status);

    status = clEnqueueAcquireGLObjects(CLU_DEFAULT_Q, 1, &m_points, 0, 0, 0);
    status = clEnqueueCopyBuffer(CLU_DEFAULT_Q, temp, m_points, 0, 0, pointsSize, 0, 0, 0);
    status = clEnqueueReleaseGLObjects(CLU_DEFAULT_Q, 1, &m_points, 0, 0, 0);
    clFinish(CLU_DEFAULT_Q); // make sure the buffer gets updated

    clReleaseMemObject(temp);
    delete [] points;

    m_velocities = clCreateBuffer(CLU_CONTEXT, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, pointsSize, velocities, &status);
    delete [] velocities;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void SimulateCL::Step(float in_deltaT)
{
    cl_int status;

    // acquire buffers
    status = clEnqueueAcquireGLObjects(CLU_DEFAULT_Q, 1, &m_points, 0, 0, 0);

    clu_enqueue_params params = CLU_DEFAULT_PARAMS;
    params.nd_range = CLU_ND1(m_numPoints);
    status = clugEnqueue_Simulate(m_cluKernel, &params,
        m_points, m_velocities, m_range, in_deltaT);

    // release buffers
    status = clEnqueueReleaseGLObjects(CLU_DEFAULT_Q, 1, &m_points, 0, 0, 0);

#if CL_DEVICE_TYPE_GPU == SIM_DEVICE_TYPE
    clFlush(CLU_DEFAULT_Q); // GPU implementations synchronize with a lower-impact flush
#else
    clFinish(CLU_DEFAULT_Q); // CPU implementations typically require finish to sync
#endif
}

//-----------------------------------------------------------------------------
// SimulateCL constructor
//-----------------------------------------------------------------------------
SimulateCL::SimulateCL()
{
    m_points = 0;
    m_velocities = 0;
}

//-----------------------------------------------------------------------------
// SimulateCL destructor
//-----------------------------------------------------------------------------
SimulateCL::~SimulateCL()
{
    Shutdown();
}

//-----------------------------------------------------------------------------
// recommend calling Shutdown before destroying the GL renderer, GL context, and window
//-----------------------------------------------------------------------------
void SimulateCL::Shutdown()
{
    // make sure CL is totally done
    clFinish(CLU_DEFAULT_Q);

    clReleaseMemObject(m_points);
    m_points = 0;

    clReleaseMemObject(m_velocities);
    m_velocities = 0;

    clReleaseKernel(m_cluKernel.m_kernel);
    m_cluKernel.m_kernel = 0;

    //-----------
    // stop CLU
    //-----------
    cluRelease();
}
