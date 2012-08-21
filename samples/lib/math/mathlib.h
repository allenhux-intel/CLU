/*
Copyright (c) 2012, Intel Corporation

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

class Matrix4;

template<typename T> class Vector2
{
public:
    union
    {
        T m_fData[2];
        struct
        {
            T x,y;
        };
    };

	Vector2<T>() {};
	explicit Vector2<T>(const T *src);
	Vector2<T>(const Vector2<T> &src);
	Vector2<T>(T x, T y);

	// implicit casting
	operator T* ();
	operator const T* () const;

	// assignment operators
	Vector2<T>& operator += (const Vector2<T> &right);
	Vector2<T>& operator -= (const Vector2<T> &right);
	Vector2<T>& operator *= (T right);
	Vector2<T>& operator /= (T right);

	// unary operators
	const Vector2<T>& operator + () const;
	Vector2<T> operator - () const;

	// binary operators
	Vector2<T> operator + (const Vector2<T> &right) const;
	Vector2<T> operator - (const Vector2<T> &right) const;
	Vector2<T> operator * (T right) const;
	Vector2<T> operator / (T right) const;

	friend Vector2<T> operator * (T left, const Vector2<T> &right);

	bool operator == (const Vector2<T> &right) const;
	bool operator != (const Vector2<T> &right) const;
};

typedef Vector2<float> Vector2s;
typedef Vector2<int>   Vector2i;

class Vector3
{
public:
    union
    {
        struct
        {
            float x,y,z;
        };
        struct
        {
            float u,v,w;
        };
        float m_fData[3];
    };

    Vector3() {};
	explicit Vector3(const float *src);
	Vector3(const Vector3 &src);
	Vector3(float x, float y, float z);

	// implicit casting
	operator float* ();
	operator const float* () const;

	// assignment operators
	Vector3& operator += (const Vector3 &right);
	Vector3& operator -= (const Vector3 &right);
	Vector3& operator *= (float right);
	Vector3& operator /= (float right);

	// unary operators
	const Vector3& operator + () const;
	Vector3 operator - () const;

	// binary operators
	Vector3 operator + (const Vector3 &right) const;
	Vector3 operator - (const Vector3 &right) const;
	Vector3 operator * (float right) const;
	Vector3 operator / (float right) const;

	friend Vector3 operator * (float left, const Vector3 &right);

	bool operator == (const Vector3 &right) const;
	bool operator != (const Vector3 &right) const;

	void TransformCoord( const Matrix4 &m );

	void TransformCoord( Vector3 &out, const Matrix4 &m ) const;

	void TransformNormal( const Matrix4 &m );

	void TransformNormal( Vector3 &out, const Matrix4 &m ) const;

    float Dot( const Vector3& InVec ) const;
    
    Vector3 Cross(const Vector3& v) const;

    float Magnitude() const;

    float MagnitudeSq() const;

    Vector3 Normalize();
};

class Vector4
{
public:
    union
    {
        float m_fData[4];
        struct
        {
            float x,y,z,w;
        };
    };

    Vector4() {};
	explicit Vector4(const float *src);
	Vector4(const Vector4 &src);
    Vector4(const Vector3& xyz, float w);
	Vector4(float x, float y, float z, float w);

	// implicit casting
	operator float* ();
	operator const float* () const;

	// assignment operators
	Vector4& operator += (const Vector4 &right);
	Vector4& operator -= (const Vector4 &right);
	Vector4& operator *= (float right);
	Vector4& operator /= (float right);

	// unary operators
	const Vector4& operator + () const;
	Vector4 operator - () const;

	// binary operators
	Vector4 operator + (const Vector4 &right) const;
	Vector4 operator - (const Vector4 &right) const;
	Vector4 operator * (float right) const;
	Vector4 operator / (float right) const;

	friend Vector4 operator * (float left, const Vector4 &right);

	bool operator == (const Vector4 &right) const;
	bool operator != (const Vector4 &right) const;


    float Magnitude() const;
    float MagnitudeSq() const;
    Vector4 Normalize();
};

class Matrix4
{
public:
	union
	{
		__declspec(align(16)) struct
		{
			float   _11, _12, _13, _14;
			float   _21, _22, _23, _24;
			float   _31, _32, _33, _34;
			float   _41, _42, _43, _44;
		};
		float m[4][4];
	};

	Matrix4() {};
	Matrix4(const float *src);
	Matrix4(const Matrix4 &src);
	Matrix4(float _11, float _12, float _13, float _14,
	        float _21, float _22, float _23, float _24,
	        float _31, float _32, float _33, float _34,
	        float _41, float _42, float _43, float _44);

	// element access
	float& operator () (unsigned int row, unsigned int col);
	float  operator () (unsigned int row, unsigned int col) const;

	// implicit casting
	operator float* ();
	operator const float* () const;

	// assignment operators
	Matrix4& operator *= (const Matrix4 &right);
	Matrix4& operator += (const Matrix4 &right);
	Matrix4& operator -= (const Matrix4 &right);
	Matrix4& operator *= (float right);
	Matrix4& operator /= (float right);

	// unary operators
	const Matrix4& operator + () const;
	Matrix4 operator - () const;

	// binary operators
	Matrix4 operator * (const Matrix4 &right) const;
	Matrix4 operator + (const Matrix4 &right) const;
	Matrix4 operator - (const Matrix4 &right) const;
	Matrix4 operator * (float right) const;
	Matrix4 operator / (float right) const;

	friend Matrix4 operator * (float left, const Matrix4 &right);

	bool operator == (const Matrix4 &right) const;
	bool operator != (const Matrix4 &right) const;

    void SetIdentity();
    bool IsIdentity() const;

    void RotationX(float angle);
    void RotationY(float angle);
    void RotationZ(float angle);

    void RotationAxis(const Vector3 *v, float angle);
    void RotationYawPitchRoll(float y, float p, float r);
    void LookAtRH(const Vector3& eye, const Vector3& at, const Vector3& up);
    void LookAtLH(const Vector3& eye, const Vector3& at, const Vector3& up);

    void ViewMatrix(const Vector3& in_xAxis, const Vector3& in_yAxis, const Vector3& in_zAxis,
        const Vector3& in_eyePosition);

    void PerspectiveRH(float w, float h, float zn, float zf);
    void PerspectiveLH(float w, float h, float zn, float zf);
    void PerspectiveFovRH(float fovy, float aspect, float zn, float zf);
    void PerspectiveFovLH(float fovy, float aspect, float zn, float zf);

    void Scaling(float sx, float sy, float sz);
    void Translation(float x, float y, float z);

    void PerspectiveOffCenterRH(float l, float r, float b, float t, float zn, float zf);
    void PerspectiveOffCenterLH(float l, float r, float b, float t, float zn, float zf);
    void OrthoRH(float w, float h, float zn, float zf);
    void OrthoLH(float w, float h, float zn, float zf);
    void OrthoOffCenterRH(float l, float r, float b, float t, float zn, float zf);
    void OrthoOffCenterLH(float l, float r, float b, float t, float zn, float zf);

    float Determinant() const;
    void  Transpose(); // in-place transpose
};

// math functions:

//
// Matrix4
//


Matrix4* MatrixMultiply(Matrix4 *out, const Matrix4 *m1, const Matrix4 *m2);

// Matrix multiplication, followed by a transpose. (Out = T(M1 * M2))
Matrix4* MatrixMultiplyTranspose(Matrix4 *out, const Matrix4 *m1, const Matrix4 *m2);

// Calculate inverse of matrix.  Inversion may fail, in which case NULL will
// be returned.  The determinant of 'm' is also returned it 'det' is non-NULL.
Matrix4* MatrixInverse(Matrix4 *out, float *det, const Matrix4 *m);


//=============================================================================
// Camera class
// use for View matrix
//=============================================================================
// _11, _21, _31 is xAxis
// _12, _22, _32 is yAxis
// _13, _23, _33 is zAxis
class Camera : public Matrix4
{
public:
    // translate using local coordinate system.
    // E.g. translating by (0,0,1) moves in the look direction
    void TranslateLocal(const Vector3& in_dir);
    void RotateTrackball(const Vector3& in_point, float in_yaw, float in_pitch);
    virtual void LookAt(Vector3 in_position, Vector3 in_lookAt, Vector3 in_up) = 0;
    virtual void RotateLocal(float in_yaw, float in_pitch) = 0;
protected:
    Vector3 m_pos;    // position
    Vector3 m_lookAt; // point in space to look at
};

//=============================================================================
// Left-handed view (e.g. DX)
//=============================================================================
class CameraLH : public Camera
{
public:
    void LookAt(Vector3 in_position, Vector3 in_lookAt, Vector3 up);
    void RotateLocal(float in_yaw, float in_pitch);
};

//=============================================================================
// Right-handed view (e.g. GL)
//=============================================================================
class CameraRH : public Camera
{
public:
    void LookAt(Vector3 in_position, Vector3 in_lookAt, Vector3 up);
    void RotateLocal(float in_yaw, float in_pitch);
};

template<typename T> inline void MATHLIB_SWAP(T A,T B) {T c=A; A=B; B=c;}

#include "Camera.inl"
#include "vector2.inl"
#include "vector3.inl"
#include "vector4.inl"
#include "matrix4.inl"

// end of file
