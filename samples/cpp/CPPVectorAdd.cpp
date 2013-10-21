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
    auto vectorAddKernel = clugCreate_vectorAdd();

    std::vector<int> inputA(numElements, 1);
    std::vector<int> inputB(numElements, 2);
    std::vector<int> output(numElements, 0xdeadbeef);
    cl::Buffer inputABuffer(begin(inputA), end(inputA), true);
    cl::Buffer inputBBuffer(begin(inputB), end(inputB), true);
    cl::Buffer outputBuffer(begin(output), end(output), false);

    cl::NDRange ndRange1(numElements);
    cl::NDRange ndRange2(numElements);
    cl::EnqueueArgs enqArgs( ndRange1, ndRange2);

    vectorAddKernel(
        enqArgs,
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
