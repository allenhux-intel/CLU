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
//  Vector3 Implementation
//
//////////////////////////////////////////////////////////////////////////////

inline Vector3::Vector3( 
    const float *pfVec )
{
    x = pfVec[ 0 ];
    y = pfVec[ 1 ];
    z = pfVec[ 2 ];
}

inline Vector3::Vector3(const Vector3 &src)
{
	x = src.x;
	y = src.y;
	z = src.z;
}

inline Vector3::Vector3( 
    float fInX, 
    float fInY,
    float fInZ )
{
    x = fInX;
    y = fInY;
    z = fInZ;
}

inline Vector3::operator float* ()
{
    return m_fData;
}

inline Vector3::operator const float* () const
{
    return m_fData;
}

inline Vector3& 
    Vector3::operator += ( 
    const Vector3& InVec )
{
    x += InVec.x;
    y += InVec.y;
    z += InVec.z;

    return *this;
}

inline Vector3& 
    Vector3::operator -= ( 
    const Vector3& InVec )
{
    x -= InVec.x;
    y -= InVec.y;
    z -= InVec.z;

    return *this;
}

inline Vector3& 
    Vector3::operator *= ( 
    float fVal )
{
    x *= fVal;
    y *= fVal;
    z *= fVal;

    return *this;
}

inline Vector3& 
    Vector3::operator /= ( 
    float fVal )
{
    x /= fVal;
    y /= fVal;
    z /= fVal;

    return *this;
}

// unary operators
inline Vector3 
    Vector3::operator - () const
{
    Vector3 Vec( *this );

    Vec.x = -Vec.x;
    Vec.y = -Vec.y;
    Vec.z = -Vec.z;

    return Vec;
}

// binary operators
inline Vector3 
    Vector3::operator + ( 
    const Vector3& InVec ) const
{
    Vector3 Vec( *this );

    Vec += InVec;

    return Vec;
}

inline Vector3 
    Vector3::operator - ( 
    const Vector3& InVec ) const
{
    Vector3 Vec( *this );

    Vec -= InVec;

    return Vec;
}

inline Vector3 
    Vector3::operator * ( 
    float fVal ) const
{
    Vector3 Vec( *this );

    Vec *= fVal;

    return Vec;
}

inline Vector3 
    Vector3::operator / ( 
    float fVal ) const
{
    Vector3 Vec( *this );

    Vec /= fVal;

    return Vec;
}

inline Vector3 operator * (float left, const Vector3 &right)
{
    return right * left;
}

inline bool 
    Vector3::operator == ( 
    const Vector3& Vec ) const
{
    return !( *this != Vec );
}

inline bool 
    Vector3::operator != ( 
    const Vector3& Vec ) const
{
    return fabs( Vec.x - x ) > FLT_EPSILON ||
        fabs( Vec.y - y ) > FLT_EPSILON ||
        fabs( Vec.z - z ) > FLT_EPSILON;
}

inline float   
    Vector3::Dot( 
    const Vector3& InVec ) const
{
    return x * InVec.x + y * InVec.y + z * InVec.z;
}

inline void
Vector3::TransformCoord( const Matrix4 &m )
{
	Vector3 result;
	TransformCoord(result, m);
	*this = result;
}

inline void
Vector3::TransformCoord( Vector3 &out, const Matrix4 &m ) const
{
	for( int j = 0; j < 3; ++j )
	{
		out.m_fData[j] = m.m[ 3 ][ j ];
		for( int i = 0; i < 3; ++i )
		{
			out.m_fData[j] += m.m[ i ][ j ] * m_fData[i];
		}
	}
}

inline void
Vector3::TransformNormal( const Matrix4 &m )
{
	Vector3 result;
	TransformNormal(result, m);
	*this = result;
}

inline void
Vector3::TransformNormal( Vector3 &out, const Matrix4 &m ) const
{
	for( int j = 0; j < 3; ++j )
	{
		out.m_fData[j] = 0;
		for( int i = 0; i < 3; ++i )
		{
			out.m_fData[j] += m.m[ i ][ j ] * m_fData[i];
		}
	}
}

inline Vector3 Vector3::Cross(const Vector3& v) const
{
    return Vector3(y*v.z-z*v.y, z*v.x-x*v.z, x*v.y-y*v.x);
}

inline float
Vector3::Magnitude() const
{
    return sqrtf( MagnitudeSq() );
}

inline float
Vector3::MagnitudeSq() const
{
    return x * x + y * y + z * z;
}

inline Vector3
Vector3::Normalize()
{
    *this /= Magnitude();
    return *this;
}
