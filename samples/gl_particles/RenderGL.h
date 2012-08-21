/*
Copyright (c) 2012, Intel Corporation

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#pragma once
#include "math/mathlib.h"
#include "display/GLContext.h"

class RenderGL
{
public:
    RenderGL();
    ~RenderGL();

    void Init(int in_numPoints, GLuint* out_pPoints);

    // mouse drag in pixels
    void Render(const Vector2s& in_rotation, const Vector2s& in_translation,
        const Vector2s& in_objRotation);
    void Resize(HWND in_hWnd);

    void Shutdown();
private:
    void CreateBuffers(int in_numPoints, GLuint* out_pPoints);
    int GetVertexSize();
    int GetIndexSize();
    static const float m_vertices[][3];
    static const GLuint m_indices[];

    GLuint m_points; // shared with CL
    GLuint m_points_tb; // for VS

    GLuint m_program;

    GLuint m_texture;
    GLuint m_vertexBuffer; // VB for the point sprites
    GLuint m_indexBuffer;  // IB for the point sprites
    GLuint m_vertexArray;

    GLuint m_textureID;          //
    GLuint m_projectionMatrixID; // "projectionMatrix"
    GLuint m_invViewID;          // "inverseMatrix"
    GLuint m_posID;              // "in_pos" in VS
    GLuint m_pointsID;           // "particlePosition"

    Matrix4  m_projectionMatrix;
    CameraRH m_viewMatrix;
    Matrix4  m_worldMatrix;

    int m_numPoints;
};
