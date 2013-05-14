#define __CL_ENABLE_EXCEPTIONS
//#define __NO_STD_STRING
#define  _VARIADIC_MAX 10
#include <CL/cl.hpp>
#include <iostream>
#include <vector>
#include "CPPVectorAddKernel.cl.h"

const int numElements = 32;

int main(void)
{
    // TODO:
    // Work in progress. Need a C++ build mode that constructs
    // the kernel directly
    // In C++ mode no need to include clu.h into the header?
    cl_int status;
    //cl::Program vectorAddProgram(clugGet_CPPVectorAddKernel_cl_h(&status));
    clug_vectorAdd cv = clugCreate_vectorAdd(&status); 
    
    // No overload takes a cl_kernel? hmm
    auto vectorAddKernel = 
        cl::make_kernel<
            cl::Buffer&,
            cl::Buffer&,
            cl::Buffer&
            >(cv.m_kernel);

    std::vector<int> inputA(numElements, 1);
    std::vector<int> inputB(numElements, 2);
    std::vector<int> output(numElements, 0xdeadbeef);
    cl::Buffer inputABuffer(begin(inputA), end(inputA), true);
    cl::Buffer inputBBuffer(begin(inputB), end(inputB), true);
    cl::Buffer outputBuffer(begin(output), end(output), false);

    vectorAddKernel(
        cl::EnqueueArgs(
            cl::NDRange(numElements),
            cl::NDRange(numElements)),
        inputABuffer,
        inputBBuffer,
        outputBuffer);

    cl::copy(outputBuffer, begin(output), end(output));

    std::cout << "Output:\n";
    for( int i = 1; i < numElements; ++i ) {
        std::cout << "\t" << output[i] << "\n";
    }
    std::cout << "\n\n";

}
