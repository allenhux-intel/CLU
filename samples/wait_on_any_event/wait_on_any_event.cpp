/*
Copyright (c) 2013, Intel Corporation

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// example of how to use cluWaitOnAnyEvent,
// which returns if any event in a list becomes CL_COMPLETE

#include <Windows.h>
#include <stdio.h>
#include "clu.h"

#define NUM_EVENTS 8
#define WHICH_EVENT 3

DWORD WINAPI ThreadFunc(void* pParam)
{
    printf("waiting thread: Waiting for event...\n");
    cluWaitOnAnyEvent((cl_event*)pParam, NUM_EVENTS);
    printf("waiting thread: Event signaled\n");
    return 0;
}

void main()
{
    cluInitialize(0);

    cl_event events[NUM_EVENTS];
    for (int i = 0; i < NUM_EVENTS; i++)
    {
        events[i] = clCreateUserEvent(cluGetContext(), 0);
    }
    // create a thread that will wait for /any/ event to fire
    DWORD threadId;
    HANDLE threadHandle = CreateThread(0, 0, ThreadFunc, events, 0, &threadId);

    printf("main thread: thread created\n");
    Sleep(1000);
    printf("main thread: signaling one of the events\n");

    clSetUserEventStatus(events[WHICH_EVENT], CL_COMPLETE);

    WaitForMultipleObjects(1, &threadHandle, TRUE, INFINITE);

    cluRelease();
}
