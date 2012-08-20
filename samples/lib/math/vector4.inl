/*
Copyright (c) 2012, Intel Corporation

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "mathlib.h"

#include <assert.h>
#include <memory>

//////////////////////////////////////////////////////////////////////////////
//
//  Vector4 Implementation
//
//////////////////////////////////////////////////////////////////////////////

inline Vector4::Vector4( 
    const float *pfVec )
{
    x = pfVec[ 0 ];
    y = pfVec[ 1 ];
    z = pfVec[ 2 ];
    w = pfVec[ 3 ];
}

inline Vector4::Vector4(const Vector4 &src)
{
	x = src.x;
	y = src.y;
	z = src.z;
	w = src.w;
}

inline Vector4::Vector4( 
    float fInX, 
    float fInY,
    float fInZ,
    float fInW )
{
    x = fInX;
    y = fInY;
    z = fInZ;
    w = fInW;
}

inline Vector4::Vector4( 
    const Vector3&  pVec, 
    float fInW )
{
    x = pVec.x;
    y = pVec.y;
    z = pVec.z;
    w = fInW;
}

inline Vector4::operator float* ()
{
    return m_fData;
}

inline Vector4::operator const float* () const
{
    return m_fData;
}

inline Vector4& 
    Vector4::operator += ( 
    const Vector4& InVec )
{
    x += InVec.x;
    y += InVec.y;
    z += InVec.z;
    w += InVec.w;

    return *this;
}

inline Vector4& 
    Vector4::operator -= ( 
    const Vector4& InVec )
{
    x -= InVec.x;
    y -= InVec.y;
    z -= InVec.z;
    w -= InVec.w;

    return *this;
}

inline Vector4& 
    Vector4::operator *= ( 
    float fVal )
{
    x *= fVal;
    y *= fVal;
    z *= fVal;
    w *= fVal;

    return *this;
}

inline Vector4& 
    Vector4::operator /= ( 
    float fVal )
{
    x /= fVal;
    y /= fVal;
    z /= fVal;
    w /= fVal;

    return *this;
}

// unary operators
inline Vector4 
    Vector4::operator - () const
{
    Vector4 Vec( *this );

    Vec.x = -Vec.x;
    Vec.y = -Vec.y;
    Vec.z = -Vec.z;
    Vec.w = -Vec.w;

    return Vec;
}

// binary operators
inline Vector4 
    Vector4::operator + ( 
    const Vector4& InVec ) const
{
    Vector4 Vec( *this );

    Vec += InVec;

    return Vec;
}

inline Vector4 
    Vector4::operator - ( 
    const Vector4& InVec ) const
{
    Vector4 Vec( *this );

    Vec -= InVec;

    return Vec;
}

inline Vector4 
    Vector4::operator * ( 
    float fVal ) const
{
    Vector4 Vec( *this );

    Vec *= fVal;

    return Vec;
}

inline Vector4 
    Vector4::operator / ( 
    float fVal ) const
{
    Vector4 Vec( *this );

    Vec /= fVal;

    return Vec;
}

inline Vector4 operator * (float left, const Vector4 &right)
{
    return right * left;
}

inline bool 
    Vector4::operator == ( 
    const Vector4& Vec ) const
{
    return !( *this != Vec );
}

inline bool 
    Vector4::operator != ( 
    const Vector4& Vec ) const
{
    return fabs( Vec.x - x ) > FLT_EPSILON ||
        fabs( Vec.y - y ) > FLT_EPSILON ||
        fabs( Vec.z - z ) > FLT_EPSILON || 
        fabs( Vec.w - w ) > FLT_EPSILON;
}

inline float
Vector4::Magnitude() const
{
    return sqrtf( MagnitudeSq() );
}

inline float
Vector4::MagnitudeSq() const
{
    return x * x + y * y + z * z + w * w;
}

inline Vector4
Vector4::Normalize()
{
    *this /= Magnitude();
    return *this;
}