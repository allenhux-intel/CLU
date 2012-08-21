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
//  Matrix4 Implementation
//
//////////////////////////////////////////////////////////////////////////////

inline Matrix4::Matrix4(const float *src)
{
    assert( src );

    memcpy(&_11, src, sizeof(Matrix4));
}

inline Matrix4::Matrix4(const Matrix4 &src)
{
    memcpy(&_11, &src, sizeof(Matrix4));
}

inline Matrix4::Matrix4(float f11, float f12, float f13, float f14,
    float f21, float f22, float f23, float f24,
    float f31, float f32, float f33, float f34,
    float f41, float f42, float f43, float f44)
{

    _11 = f11; _12 = f12; _13 = f13; _14 = f14;
    _21 = f21; _22 = f22; _23 = f23; _24 = f24;
    _31 = f31; _32 = f32; _33 = f33; _34 = f34;
    _41 = f41; _42 = f42; _43 = f43; _44 = f44;
}

inline float& Matrix4::operator () ( unsigned int row, unsigned int col )
{
    assert( row<4 && col<4 );

    return m[row][col];
}

inline float  Matrix4::operator () ( unsigned int row, unsigned int col ) const
{
    assert( row<4 && col<4 );

    return m[row][col];
}

inline Matrix4::operator float* ()
{
    return (float*)*m;
}

inline Matrix4::operator const float* () const
{
    return (const float*)*m;
}

inline Matrix4& Matrix4::operator *= (const Matrix4 &right)
{
    MatrixMultiply(this, this, &right);
    return *this;
}

inline Matrix4& Matrix4::operator += (const Matrix4 &right)
{
    _11 += right._11; _12 += right._12; _13 += right._13; _14 += right._14;
    _21 += right._21; _22 += right._22; _23 += right._23; _24 += right._24;
    _31 += right._31; _32 += right._32; _33 += right._33; _34 += right._34;
    _41 += right._41; _42 += right._42; _43 += right._43; _44 += right._44;
    return *this;
}

inline Matrix4& Matrix4::operator -= (const Matrix4 &right)
{
    _11 -= right._11; _12 -= right._12; _13 -= right._13; _14 -= right._14;
    _21 -= right._21; _22 -= right._22; _23 -= right._23; _24 -= right._24;
    _31 -= right._31; _32 -= right._32; _33 -= right._33; _34 -= right._34;
    _41 -= right._41; _42 -= right._42; _43 -= right._43; _44 -= right._44;

    return *this;
}
inline Matrix4& Matrix4::operator *= (float right)
{
    _11 *= right; _12 *= right; _13 *= right; _14 *= right;
    _21 *= right; _22 *= right; _23 *= right; _24 *= right;
    _31 *= right; _32 *= right; _33 *= right; _34 *= right;
    _41 *= right; _42 *= right; _43 *= right; _44 *= right;

    return *this;
}
inline Matrix4& Matrix4::operator /= (float right)
{
    float rightInv = 1.0f / right;
    _11 *= rightInv; _12 *= rightInv; _13 *= rightInv; _14 *= rightInv;
    _21 *= rightInv; _22 *= rightInv; _23 *= rightInv; _24 *= rightInv;
    _31 *= rightInv; _32 *= rightInv; _33 *= rightInv; _34 *= rightInv;
    _41 *= rightInv; _42 *= rightInv; _43 *= rightInv; _44 *= rightInv;

    return *this;
}

inline const Matrix4& Matrix4::operator + () const
{
    return *this;
}

inline Matrix4 Matrix4::operator - () const
{
    return Matrix4(-_11, -_12, -_13, -_14,
        -_21, -_22, -_23, -_24,
        -_31, -_32, -_33, -_34,
        -_41, -_42, -_43, -_44);
}

inline Matrix4 Matrix4::operator * (const Matrix4 &right) const
{
    Matrix4 res;
    MatrixMultiply(&res, this, &right);
    return res;
}

inline Matrix4 Matrix4::operator + (const Matrix4 &right) const
{
    return Matrix4(_11 + right._11, _12 + right._12, _13 + right._13, _14 + right._14,
        _21 + right._21, _22 + right._22, _23 + right._23, _24 + right._24,
        _31 + right._31, _32 + right._32, _33 + right._33, _34 + right._34,
        _41 + right._41, _42 + right._42, _43 + right._43, _44 + right._44);
}

inline Matrix4 Matrix4::operator - (const Matrix4 &right) const
{
    return Matrix4(_11 - right._11, _12 - right._12, _13 - right._13, _14 - right._14,
        _21 - right._21, _22 - right._22, _23 - right._23, _24 - right._24,
        _31 - right._31, _32 - right._32, _33 - right._33, _34 - right._34,
        _41 - right._41, _42 - right._42, _43 - right._43, _44 - right._44);
}

inline Matrix4 Matrix4::operator * (float right) const
{
    return Matrix4(_11 * right, _12 * right, _13 * right, _14 * right,
        _21 * right, _22 * right, _23 * right, _24 * right,
        _31 * right, _32 * right, _33 * right, _34 * right,
        _41 * right, _42 * right, _43 * right, _44 * right);
}

inline Matrix4 Matrix4::operator / (float right) const
{
    float rightInv = 1.0f / right;
    return Matrix4(_11 * rightInv, _12 * rightInv, _13 * rightInv, _14 * rightInv,
        _21 * rightInv, _22 * rightInv, _23 * rightInv, _24 * rightInv,
        _31 * rightInv, _32 * rightInv, _33 * rightInv, _34 * rightInv,
        _41 * rightInv, _42 * rightInv, _43 * rightInv, _44 * rightInv);
}

inline Matrix4 operator * (float left, const Matrix4 &right)
{
    return Matrix4(left * right._11, left * right._12, left * right._13, left * right._14,
        left * right._21, left * right._22, left * right._23, left * right._24,
        left * right._31, left * right._32, left * right._33, left * right._34,
        left * right._41, left * right._42, left * right._43, left * right._44);
}

inline bool Matrix4::operator == (const Matrix4 &right) const
{
    return 0 == memcmp(this, &right, sizeof(Matrix4));
}

inline bool Matrix4::operator != (const Matrix4 &right) const
{
    return 0 != memcmp(this, &right, sizeof(Matrix4));
}

inline void Matrix4::SetIdentity()
{
    m[0][1] = m[0][2] = m[0][3] =
        m[1][0] = m[1][2] = m[1][3] =
        m[2][0] = m[2][1] = m[2][3] =
        m[3][0] = m[3][1] = m[3][2] = 0.0f;

    m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.0f;
}

inline bool Matrix4::IsIdentity() const
{
    Matrix4 identity;
    identity.SetIdentity();
    return identity == *this;
}

inline void Matrix4::RotationX(float angle)
{
    float sinAngle = sinf(angle);
    float cosAngle = cosf(angle);

    SetIdentity();

    m[1][1] = cosAngle;
    m[1][2] = sinAngle;
    m[2][1] = -sinAngle;	
    m[2][2] = cosAngle;
}

inline void Matrix4::RotationY(float angle)
{
    float sinAngle = sinf(angle);
    float cosAngle = cosf(angle);

    SetIdentity();

    m[0][0] = cosAngle;
    m[0][2] = -sinAngle;
    m[2][0] = sinAngle;	
    m[2][2] = cosAngle;
}

inline void Matrix4::RotationZ(float angle)
{
    float sinAngle = sinf(angle);
    float cosAngle = cosf(angle);

    SetIdentity();

    m[0][0] = cosAngle;
    m[0][1] = sinAngle;
    m[1][0] = -sinAngle;
    m[1][1] = cosAngle;
}

inline void Matrix4::RotationAxis(const Vector3 *v, float angle)
{
    assert(v);

    Vector3 vnorm = *v;
    vnorm.Normalize();
    float sinAngle = sinf(angle);
    float cosAngle = cosf(angle);

    SetIdentity();

    m[0][0] = cosAngle + (1.0f - cosAngle) * vnorm.x * vnorm.x;
    m[0][1] = (1.0f - cosAngle) * vnorm.y * vnorm.x + sinAngle * vnorm.z;
    m[0][2] = (1.0f - cosAngle) * vnorm.z * vnorm.x - sinAngle * vnorm.y;
    m[1][0] = (1.0f - cosAngle) * vnorm.x * vnorm.y - sinAngle * vnorm.z;	
    m[1][1] = cosAngle + (1.0f - cosAngle)*vnorm.y*vnorm.y;
    m[1][2] = (1.0f - cosAngle) * vnorm.z * vnorm.y + sinAngle * vnorm.x;
    m[2][0] = (1.0f - cosAngle) * vnorm.x * vnorm.z + sinAngle * vnorm.y;
    m[2][1] = (1.0f - cosAngle) * vnorm.y * vnorm.z - sinAngle * vnorm.x;
    m[2][2] = cosAngle + (1.0f - cosAngle)*vnorm.z*vnorm.z;
}

inline void Matrix4::RotationYawPitchRoll(float y, float p, float r)
{
    float sinY = sinf(y);
    float sinP = sinf(p);
    float sinR = sinf(r);
    float cosY = cosf(y);
    float cosP = cosf(p);
    float cosR = cosf(r);

    SetIdentity();	

    m[0][0] = sinY * sinP * sinR + cosY * cosR;
    m[1][0] = sinY * sinP * cosR - cosY * sinR;
    m[2][0] = sinY * cosP;
    m[0][1] = cosP * sinR;
    m[1][1] = cosP * cosR;
    m[2][1] =-sinP;
    m[0][2] = cosY * sinP * sinR  -sinY * cosR;
    m[1][2] = cosY * sinP * cosR + sinY * sinR;
    m[2][2] = cosY * cosP;
}

inline void Matrix4::ViewMatrix(const Vector3& in_xAxis, const Vector3& in_yAxis, const Vector3& in_zAxis,
    const Vector3& in_eyePosition)
{
    _11 = in_xAxis.x;
    _12 = in_yAxis.x;
    _13 = in_zAxis.x;
    _14 = 0.0f;
    _21 = in_xAxis.y;
    _22 = in_yAxis.y;
    _23 = in_zAxis.y;
    _24 = 0.0f;
    _31 = in_xAxis.z;
    _32 = in_yAxis.z;
    _33 = in_zAxis.z;
    _34 = 0.0f;
    _41 = -in_xAxis.Dot(in_eyePosition);
    _42 = -in_yAxis.Dot(in_eyePosition);
    _43 = -in_zAxis.Dot(in_eyePosition);
    _44 = 1.0f;
}

inline void Matrix4::LookAtRH(const Vector3& eye, const Vector3& at, const Vector3& up)
{
    Vector3 zaxis = eye - at;
    zaxis.Normalize();

    Vector3 xaxis = up.Cross(zaxis);
    xaxis.Normalize();

    Vector3 yaxis = zaxis.Cross(xaxis);

    ViewMatrix(xaxis, yaxis, zaxis, eye);
}

inline void Matrix4::LookAtLH(const Vector3& eye, const Vector3& at, const Vector3& up)
{
    Vector3 zaxis = at - eye;
    zaxis.Normalize();

    Vector3 xaxis = up.Cross(zaxis);
    xaxis.Normalize();

    Vector3 yaxis = zaxis.Cross(xaxis);

    ViewMatrix(xaxis, yaxis, zaxis, eye);
}

inline void Matrix4::PerspectiveRH(float w, float h, float zn, float zf)
{
    _11 = 2.0f * zn / w;
    _12 = 0.0f;
    _13 = 0.0f;
    _14 = 0.0f;
    _21 = 0.0f;
    _22 = 2.0f * zn / h;
    _23 = 0.0f;
    _24 = 0.0f;
    _31 = 0.0f;
    _32 = 0.0f;
    _33 = zf / (zn - zf);
    _34 =-1.0f;
    _41 = 0.0f;
    _42 = 0.0f;
    _43 = zn * zf / (zn - zf);
    _44 = 0.0f;
}

inline void Matrix4::PerspectiveLH(float w, float h, float zn, float zf)
{
    _11 = 2.0f * zn / w;
    _12 = 0.0f;
    _13 = 0.0f;
    _14 = 0.0f;
    _21 = 0.0f;
    _22 = 2.0f * zn / h;
    _23 = 0.0f;
    _24 = 0.0f;
    _31 = 0.0f;
    _32 = 0.0f;
    _33 = zf / (zf - zn);
    _34 = 1.0f;
    _41 = 0.0f;
    _42 = 0.0f;
    _43 = zn * zf / (zn - zf);
    _44 = 0.0f;
}

inline void Matrix4::PerspectiveFovRH(float fovy, float aspect, float zn, float zf)
{
    float xScale, yScale;
    yScale = cosf(fovy/2.0f) / sinf(fovy/2.0f);
    xScale = yScale / aspect ;

    _11 = xScale;
    _12 = 0.0f;
    _13 = 0.0f;
    _14 = 0.0f;
    _21 = 0.0f;
    _22 = yScale;
    _23 = 0.0f;
    _24 = 0.0f;
    _31 = 0.0f;
    _32 = 0.0f;
    _33 = zf / (zn-zf);
    _34 = -1.0f;
    _41 = 0.0f;
    _42 = 0.0f;
    _43 = zn*zf / (zn-zf);
    _44 = 0.0f;
}

inline void Matrix4::PerspectiveFovLH(float fovy, float aspect, float zn, float zf)
{
    float xScale, yScale;
    yScale = cosf(fovy/2.0f) / sinf(fovy/2.0f);
    xScale = yScale / aspect ;

    _11 = xScale;
    _12 = 0.0f;
    _13 = 0.0f;
    _14 = 0.0f;
    _21 = 0.0f;
    _22 = yScale;
    _23 = 0.0f;
    _24 = 0.0f;
    _31 = 0.0f;
    _32 = 0.0f;
    _33 = zf/(zf-zn);
    _34 = 1.0f;
    _41 = 0.0f;
    _42 = 0.0f;
    _43 = -zn*zf/(zf-zn);
    _44 = 0.0f;
}

inline void Matrix4::Scaling(float sx, float sy, float sz)
{
    SetIdentity();

    m[0][0] = sx;
    m[1][1] = sy;
    m[2][2] = sz;
}

inline void Matrix4::Translation(float x, float y, float z)
{
    SetIdentity();

    m[3][0] = x;
    m[3][1] = y;
    m[3][2] = z;
}

inline void Matrix4::PerspectiveOffCenterRH(float l, float r, float b, float t, float zn, float zf)
{
    _11 = 2.0f * zn / (r - l);
    _12 = 0.0f;
    _13 = 0.0f;
    _14 = 0.0f;
    _21 = 0.0f;
    _22 = 2.0f * zn / (t - b);
    _23 = 0.0f;
    _24 = 0.0f;
    _31 = (l + r) / (r - l);
    _32 = (t + b) / (t - b);
    _33 = zf / (zn - zf);
    _34 =-1.0f;
    _41 = 0.0f;
    _42 = 0.0f;
    _43 = zn * zf/(zn - zf);
    _44 = 0.0f;
}

inline void Matrix4::PerspectiveOffCenterLH(float l, float r, float b, float t, float zn, float zf)
{
    _11 = 2.0f * zn / (r - l);
    _12 = 0.0f;
    _13 = 0.0f;
    _14 = 0.0f;
    _21 = 0.0f;
    _22 = 2.0f * zn / (t - b);
    _23 = 0.0f;
    _24 = 0.0f;
    _31 = (l + r) / (l - r);
    _32 = (t + b) / (b - t);
    _33 = zf / (zf - zn);
    _34 = 1.0f;
    _41 = 0.0f;
    _42 = 0.0f;
    _43 = zn * zf/(zn - zf);
    _44 = 0.0f;
}

inline void Matrix4::OrthoRH(float w, float h, float zn, float zf)
{
    _11 = 2 / w;
    _12 = 0.0f;
    _13 = 0.0f;
    _14 = 0.0f;
    _21 = 0.0f;
    _22 = 2 / h;
    _23 = 0.0f;
    _24 = 0.0f;
    _31 = 0.0f;
    _32 = 0.0f;
    _33 = 1.0f / (zn-zf);
    _34 = 0.0f;
    _41 = 0.0f;
    _42 = 0.0f;
    _43 = zn / (zn - zf);
    _44 = 1.0f;
}

inline void Matrix4::OrthoLH(float w, float h, float zn, float zf)
{
    _11 = 2.0f / w;
    _12 = 0.0f;
    _13 = 0.0f;
    _14 = 0.0f;
    _21 = 0.0f;
    _22 = 2.0f / h;
    _23 = 0.0f;
    _24 = 0.0f;
    _31 = 0.0f;
    _32 = 0.0f;
    _33 = 1.0f / (zf - zn);
    _34 = 0.0f;
    _41 = 0.0f;
    _42 = 0.0f;
    _43 = zn / (zn - zf);
    _44 = 1.0f;
}

inline void Matrix4::OrthoOffCenterRH(float l, float r, float b, float t, float zn, float zf)
{
    _11 = 2.0f / (r - l);
    _12 = 0.0f;
    _13 = 0.0f;
    _14 = 0.0f;
    _21 = 0.0f;
    _22 = 2.0f / (t - b);
    _23 = 0.0f;
    _24 = 0.0f;
    _31 = 0.0f;
    _32 = 0.0f;
    _33 = 1.0f / (zn - zf);
    _34 = 0.0f;
    _41 = (l + r) / (l - r);
    _42 = (t + b) / (b - t);
    _43 = zn / ( zn - zf);
    _44 = 1.0f;
}

inline void Matrix4::OrthoOffCenterLH(float l, float r, float b, float t, float zn, float zf)
{
    _11 = 2.0f / (r - l);
    _12 = 0.0f;
    _13 = 0.0f;
    _14 = 0.0f;
    _21 = 0.0f;
    _22 = 2.0f / (t - b);
    _23 = 0.0f;
    _24 = 0.0f;
    _31 = 0.0f;
    _32 = 0.0f;
    _33 = 1.0f / (zf - zn);
    _34 = 0.0f;
    _41 = (l + r) / (l - r);
    _42 = (t + b) / (b - t);
    _43 = zn / ( zn - zf);
    _44 = 1.0f;
}

inline float Matrix4::Determinant() const
{
    return _11*(_22 * (_33*_44 - _34*_43) - _23*(_32*_44-_42*_34) + _24*(_32*_43 - _33*_42)) -
        _12*(_21 * (_33*_44 - _34*_43) - _23*(_31*_44-_41*_34) + _24*(_31*_43 - _33*_41)) +
        _13*(_21 * (_32*_44 - _34*_42) - _22*(_31*_44-_41*_34) + _24*(_31*_42 - _32*_41)) -
        _14*(_21 * (_32*_43 - _33*_42) - _22*(_31*_43-_41*_33) + _23*(_31*_42 - _32*_41));
}

inline void Matrix4::Transpose()
{
    MATHLIB_SWAP(m[1][0],m[0][1]);
    MATHLIB_SWAP(m[2][0],m[0][2]);
    MATHLIB_SWAP(m[3][0],m[0][3]);
    MATHLIB_SWAP(m[1][2],m[2][1]);
    MATHLIB_SWAP(m[1][3],m[3][1]);
    MATHLIB_SWAP(m[2][3],m[3][2]);
}

inline Matrix4* MatrixMultiply(Matrix4 *out, const Matrix4 *m1, const Matrix4 *m2)
{
    assert(out && m1 && m2);
    Matrix4 res;
    for( int i = 0; i < 4; ++i )
    {
        res.m[i][0] = m1->m[i][0]*m2->m[0][0] + m1->m[i][1]*m2->m[1][0] + m1->m[i][2]*m2->m[2][0] + m1->m[i][3]*m2->m[3][0];
        res.m[i][1] = m1->m[i][0]*m2->m[0][1] + m1->m[i][1]*m2->m[1][1] + m1->m[i][2]*m2->m[2][1] + m1->m[i][3]*m2->m[3][1];
        res.m[i][2] = m1->m[i][0]*m2->m[0][2] + m1->m[i][1]*m2->m[1][2] + m1->m[i][2]*m2->m[2][2] + m1->m[i][3]*m2->m[3][2];
        res.m[i][3] = m1->m[i][0]*m2->m[0][3] + m1->m[i][1]*m2->m[1][3] + m1->m[i][2]*m2->m[2][3] + m1->m[i][3]*m2->m[3][3];
    }
    *out = res;
    return out;
}

// Matrix multiplication, followed by a transpose. (Out = T(M1 * M2))
inline Matrix4* MatrixMultiplyTranspose(Matrix4 *out, const Matrix4 *m1, const Matrix4 *m2)
{
    assert( out && m1 && m2 );
    Matrix4 res;
    for( unsigned int i=0; i<4; ++i )
    {
        for( unsigned int j=0; j<4; ++j )
        {
            res.m[j][i] = m1->m[i][0]*m2->m[0][j] + m1->m[i][1]*m2->m[1][j] + m1->m[i][2]*m2->m[2][j] + m1->m[i][3]*m2->m[3][j];
        }
    }
    *out = res;
    return out;
}

/// Calculate inverse of matrix.  Inversion my fail, in which case NULL will
/// be returned.  The determinant of 'm' is also returned it 'det' is non-NULL.
inline Matrix4* MatrixInverse(Matrix4 *out, float *det, const Matrix4 *m)
{
    assert(out && m);
    Matrix4 copy = *m;
    Matrix4 res;

    float eps = std::numeric_limits<float>::epsilon() * 12.0f;

    float mdet=1.0f;
    float maxVal;
    unsigned int indMax[4];
    res.SetIdentity();
    for( unsigned int i = 0; i < 4; ++i )
    {
        indMax[i]=i;
    }
    for( unsigned int i=0; i<4; ++i )
    {
        float max = fabs(copy.m[i][indMax[i]]);
        unsigned int maxValInd = indMax[i];
        for( unsigned int j=i+1; j<4; ++j )
        {
            if( fabs(copy.m[i][indMax[j]]) > max )
            {
                maxValInd = j;
                max = fabs(copy.m[i][indMax[j]]);
            }
        }

        // checking for singular matrix
        if( max < eps )
        {
            if(det)
                *det = 0;
            return NULL;	
        }

        if( maxValInd!=indMax[i] )
            std::swap( indMax[i], indMax[maxValInd] );
        mdet *= copy.m[i][indMax[i]];
        maxVal = copy.m[i][indMax[i]];

        for( unsigned int j=0; j<4; ++j )
        {
            copy.m[i][indMax[j]] /= maxVal;
            res.m[i][indMax[j]] /= maxVal;
        }
        for( unsigned int l=0; l<4; ++l )
        {
            if( i!=l )
            {
                float prin_col_val = copy.m[l][indMax[i]];
                for( unsigned int j=i; j<4; ++j )
                {
                    copy.m[l][indMax[j]] -= copy.m[i][indMax[j]]*prin_col_val;
                }
                for( unsigned int j=0; j<4; ++j )
                {
                    res.m[l][j] -= res.m[i][j]*prin_col_val;
                }
            }
        }
    }
    int shift_count=0;
    for( unsigned int i=0; i<4; ++i )
    {
        for( unsigned int j=0; j<4; ++j )
        {
            out->m[indMax[i]][j] = res.m[i][j];
        }
    }
    if(det)
    {
        for( unsigned int i=0; i<4; ++i )
        {
            for( unsigned int j=i+1; j<4; ++j )
            {
                if(indMax[j] == i)
                {
                    ++shift_count;
                    std::swap(indMax[j], indMax[i]);
                }
            }
        }
        *det = shift_count%2 == 0 ? mdet : -mdet;
    }
    return out;

}
