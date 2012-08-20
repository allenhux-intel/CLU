
This gl_particles sample was shown at SIGGRAPH 2012 as part of the OpenCL* BOF.
It demonstrates using CLU to create an OpenCL* context from an OpenGL* context.
The SimulateCL class encapsulates the OpenCL* code used to animate
the particle positions stored in a buffer shared with OpenGL*.

In this sample, there is only 1 source of gravity: the center of the universe.
Particle speed is used to determine color.



Not all platforms support OpenGL* sharing with OpenCL*. Also, in some cases,
the CPU OpenCL* device works more reliably. You can request CLU to use a CPU
device by changing the following #define in SimulateCL.cpp:

#define SIM_DEVICE_TYPE CL_DEVICE_TYPE_CPU



Navigation:

mouse + left button: rotate camera
mouse + right button: rotate universe
arrow keys: translate camera
shift + arrow keys: translate camera faster




*Other names and brands may be claimed as the property of others. 
OpenCL and the OpenCL logo are trademarks of Apple Inc. used by permission by Khronos.
