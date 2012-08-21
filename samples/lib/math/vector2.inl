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
//  Vector2 Implementation
//
//////////////////////////////////////////////////////////////////////////////

template<typename T> inline Vector2<T>::Vector2(const T *src)
{
	assert(src);

	x = src[0];
	y = src[1];
}

template<typename T> inline Vector2<T>::Vector2(const Vector2<T> &src)
{
	x = src.x;
	y = src.y;
}

template<typename T> inline Vector2<T>::Vector2(T x, T y)
{
	this->x = x;
	this->y = y;
}

template<typename T> inline Vector2<T>::operator T* ()
{
	return m_fData;
}

template<typename T> inline Vector2<T>::operator const T* () const
{
	return m_fData;
}

template<typename T> inline Vector2<T>& Vector2<T>::operator += (const Vector2<T> &right)
{
	x+=right.x;
	y+=right.y;

	return *this;
}

template<typename T> inline Vector2<T>& Vector2<T>::operator -= (const Vector2<T> &right)
{
	x-=right.x;
	y-=right.y;

	return *this;
}

template<typename T> inline Vector2<T>& Vector2<T>::operator *= (T right)
{
	x*=right;
	y*=right;

	return *this;
}

template<typename T> inline Vector2<T>& Vector2<T>::operator /= (T right)
{
	x/=right;
	y/=right;

	return *this;
}

template<typename T> inline const Vector2<T>& Vector2<T>::operator + () const
{
	return *this;
}

template<typename T> inline Vector2<T> Vector2<T>::operator - () const
{
	Vector2 res(-x, -y);

	return res;
}

template<typename T> inline Vector2<T> Vector2<T>::operator + (const Vector2<T> &right) const
{
	Vector2 res;
	res.x = x + right.x;
	res.y = y + right.y;

	return res;
}

template<typename T> inline Vector2<T> Vector2<T>::operator - (const Vector2<T> &right) const
{
	Vector2 res;
	res.x = x - right.x;
	res.y = y - right.y;

	return res;
}

template<typename T> inline Vector2<T> Vector2<T>::operator * (T right) const
{
	Vector2 res;
	res.x = x * right;
	res.y = y * right;

	return res;
}

template<typename T> inline Vector2<T> Vector2<T>::operator / (T right) const
{
	Vector2 res;
	res.x = x / right;
	res.y = y / right;

	return res;
}


template<typename T> inline Vector2<T> operator * (T left, const Vector2<T> &right)
{
	Vector2 res;
	res.x = left * right.x;
	res.y = left * right.y;

	return res;
}

template<typename T> inline bool Vector2<T>::operator == (const Vector2<T> &right) const
{
	return x==right.x && y==right.y;
}

template<typename T> inline bool Vector2<T>::operator != (const Vector2<T> &right) const
{
	return x!=right.x || y!=right.y;
}
