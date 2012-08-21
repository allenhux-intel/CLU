/*
Copyright (c) 2012, Intel Corporation

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#pragma once
#include "math\mathlib.h"

//-----------------------------------------------------------------------------
// LH is the same as RH except for Z
//-----------------------------------------------------------------------------
inline void CameraLH::LookAt(Vector3 in_position, Vector3 in_lookAt, Vector3 in_up)
{
    m_pos = in_position;
    m_lookAt = in_lookAt;

    LookAtLH(m_pos, m_lookAt, in_up);
}

//-----------------------------------------------------------------------------
// RH is the same as LH except for Z
//-----------------------------------------------------------------------------
inline void CameraRH::LookAt(Vector3 in_position, Vector3 in_lookAt, Vector3 in_up)
{
    m_pos = in_position;
    m_lookAt = in_lookAt;

    LookAtRH(m_pos, m_lookAt, in_up);
}

//-----------------------------------------------------------------------------
// shared function re-uses virtual Init()
// scale the transform axes by the input vector, then add to the position/lookat
//-----------------------------------------------------------------------------
inline void Camera::TranslateLocal(const Vector3& in_dir)
{
    Vector3 xAxis(_11, _21, _31);
    Vector3 yAxis(_12, _22, _32);
    Vector3 zAxis(_13, _23, _33);
    Vector3 translation = xAxis * in_dir.x;
    translation += yAxis * in_dir.y;
    translation += zAxis * in_dir.z;
    Vector3 pos = m_pos + translation;
    Vector3 lookAt = m_lookAt + translation;

    LookAt(pos, lookAt, yAxis);
}

//-----------------------------------------------------------------------------
// rotate around camera position
//-----------------------------------------------------------------------------
inline void CameraRH::RotateLocal(float in_yaw, float in_pitch)
{
    Vector3 xAxis(_11, _21, _31);
    Vector3 yAxis(_12, _22, _32);
    Vector3 zAxis(_13, _23, _33);

    Matrix4 ry;
    ry.RotationAxis(&yAxis, in_yaw);

    Matrix4 rx;
    rx.RotationAxis(&xAxis, in_pitch);

    zAxis.TransformNormal(ry);
    zAxis.TransformNormal(rx);
    yAxis.TransformNormal(rx);

    // if normalized zAxis is way off in magnitude from
    // the position vector, then when adding them one
    // may overwhelm the other. Get them to ~same size:
    float scaleFactor = m_pos.Magnitude();
    Vector3 scaledZ = (zAxis * scaleFactor);
    Vector3 lookAt = m_pos - scaledZ;

    LookAt(m_pos, lookAt, yAxis);
}

inline void CameraLH::RotateLocal(float in_yaw, float in_pitch)
{
    Vector3 xAxis(_11, _21, _31);
    Vector3 yAxis(_12, _22, _32);
    Vector3 zAxis(_13, _23, _33);

    Matrix4 ry;
    ry.RotationAxis(&yAxis, in_yaw);

    Matrix4 rx;
    rx.RotationAxis(&xAxis, in_pitch);

    zAxis.TransformNormal(ry);
    zAxis.TransformNormal(rx);
    yAxis.TransformNormal(rx);

    // if normalized zAxis is way off in magnitude from
    // the position vector, then when adding them one
    // may overwhelm the other. Get them to ~same size:
    float scaleFactor = m_pos.Magnitude();
    Vector3 scaledZ = (zAxis * scaleFactor);
    Vector3 lookAt = m_pos + scaledZ;

    LookAt(m_pos, lookAt, yAxis);
}

//-----------------------------------------------------------------------------
// rotate around point
//-----------------------------------------------------------------------------
inline void Camera::RotateTrackball(const Vector3& in_point, float in_yaw, float in_pitch)
{
#if 0
    // FIXME: currently only works around origin
    Vector3 translate = in_point - m_pos;
    Matrix4 t0, t1;
    t0.Translation(translate.x, translate.y, translate.z);
    t1.Translation(-translate.x, -translate.y, -translate.z);
#endif
    Vector3 xAxis(_11, _21, _31);
    Vector3 yAxis(_12, _22, _32);

    Matrix4 ry;
    ry.RotationAxis(&yAxis, in_yaw);

    Matrix4 rx;
    rx.RotationAxis(&xAxis, in_pitch);

    Matrix4 trt = ry * rx;

    m_pos.TransformCoord(trt);
    m_lookAt.TransformCoord(trt);
    yAxis.TransformNormal(trt);

    LookAt(m_pos, m_lookAt, yAxis);
}
