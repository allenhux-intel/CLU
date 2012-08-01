/*
Copyright (c) 2012, Intel Corporation

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "chdrdata.h"

__kernel void ToneMappingPerPixel(
    __global float4* inputImage, 
    __global float4* outputImage,
    __global CHDRData* pData,
    int iImageWidth
    )
{
    // Load tone mapping parameters
    float fPowKLow     = pData->fPowKLow;
    float fFStops      = pData->fFStops;
    float fFStopsInv   = pData->fFStopsInv;
    float fPowExposure = pData->fPowExposure;
    float fDefog       = pData->fDefog;
    float fGamma       = pData->fGamma;
    float fPowGamma    = pData->fPowGamma;

    // and define method constants.
    float fSaturate = 1.0f;
    
    int iRow = get_global_id(0);
    int iCol = get_global_id(1);

    float4 fColor = inputImage[iRow*iImageWidth+iCol];

    //Defog 
    fColor = fColor - fDefog;
    fColor = max(0.0f, fColor);


    // Multiply color by pow( 2.0f, exposure +  2.47393f )
    fColor = fColor * fPowExposure;

    int4 iCmpFlag = fColor > fPowKLow;
    if(any(iCmpFlag))
    {
        //fPowKLow = 2^kLow
        //fFStopsInv = 1/fFStops;
        //fTmpPixel = fPowKLow + log((fTmpPixel-fPowKLow) * fFStops + 1.0f)*fFStopsInv;
        float4 fTmpPixel = fColor - fPowKLow;
        fTmpPixel = fTmpPixel * fFStops;
        fTmpPixel = fTmpPixel + 1.0f;
        fTmpPixel = native_log( fTmpPixel );
        fTmpPixel = fTmpPixel * fFStopsInv;
        fTmpPixel = fTmpPixel + fPowKLow;

        //color channels update versions
        ///fColor = select(fTmpPixel, fColor, iCmpFlag);
        ///fColor = select(fColor, fTmpPixel, iCmpFlag);
        fColor = fTmpPixel;
    }

    //Gamma correction
    fColor = powr(fColor, fGamma);

    // Scale the values
    fColor = fColor * fSaturate;
    fColor = fColor * fPowGamma;

    //Saturate
    fColor = clamp(fColor, 0.0f, fSaturate);

    //Store result pixel 
    outputImage[iRow*iImageWidth+iCol] = fColor;
}
