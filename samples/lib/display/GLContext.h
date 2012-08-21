/*
Copyright (c) 2012, Intel Corporation

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string.h>
#include <fstream>
#include <GL/glew.h>
#include <GL/gl.h>
#include <assert.h>

#define WGL_CONTEXT_MAJOR_VERSION_ARB          0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB          0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB            0x2093
#define WGL_CONTEXT_FLAGS_ARB                  0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB           0x9126
#define WGL_CONTEXT_DEBUG_BIT_ARB              0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x0002

#define LOAD_GL_ARB(X) X##_fn X = (X##_fn)wglGetProcAddress(#X);

typedef HGLRC (WINAPI * wglCreateContextAttribsARB_fn) (HDC, HGLRC, const int *);

class GLContext
{
public:
    GLContext();
    ~GLContext();
    void Init(HWND in_hWnd, int in_major = 1, int in_minor = 1);
    void Resize();
    void Flush()   {glFlush();}
    void Present() {SwapBuffers(m_hDC);}
    void GetRC(HGLRC* out_pRC, HDC* out_pDC);
    static GLuint LoadProgram(char* in_vertexPath, char* in_fragmentPath, std::string* out_pBuildLog = 0);
    static GLuint LoadShader(char* in_path, GLuint in_shaderType, std::string* out_pBuildLog = 0);

    void Shutdown();
protected:

private:
    HWND  m_hWnd;
    HDC   m_hDC;
    HGLRC m_context;
};

//-----------------------------------------------------------------------------
// constructor
//-----------------------------------------------------------------------------
inline GLContext::GLContext()
{
    m_context = 0;
}

//-----------------------------------------------------------------------------
// destructor
//-----------------------------------------------------------------------------
inline GLContext::~GLContext()
{
    assert(0 == m_context);
}

//-----------------------------------------------------------------------------
// recommend calling Shutdown the window
//-----------------------------------------------------------------------------
inline void GLContext::Shutdown()
{
    if (m_context)
    {
        wglMakeCurrent(m_hDC, 0);
        wglDeleteContext(m_context);
        m_context = 0;
    }
}

//-----------------------------------------------------------------------------
// return handles useful for GL/CL sharing
//-----------------------------------------------------------------------------
inline void GLContext::GetRC(HGLRC* out_pRC, HDC* out_pDC)
{
    *out_pRC = m_context;
    *out_pDC = m_hDC;
}

//-----------------------------------------------------------------------------
// initialize GL context from HWND
// if major > 1 or minor > 1, creates specified context version #
//-----------------------------------------------------------------------------
inline void GLContext::Init(HWND in_hWnd, int in_major, int in_minor)
{
    m_hWnd = in_hWnd;
    PIXELFORMATDESCRIPTOR pfd =
    {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
        PFD_TYPE_RGBA,
        32,               // color depth
        0, 0, 0, 0, 0, 0,
        0,
        0,
        0,
        0, 0, 0, 0,
        32, // 32-bit depth
        0,  // no stencil
        0,
        PFD_MAIN_PLANE,
        0,
        0, 0, 0
    };
    m_hDC = GetDC(in_hWnd);
    int format = ChoosePixelFormat(m_hDC, &pfd);
    BOOL success = SetPixelFormat(m_hDC, format, &pfd);

    m_context = wglCreateContext(m_hDC);
    success = wglMakeCurrent(m_hDC, m_context);

    // create a specified context version
    if ((in_major > 1) || (in_minor > 1)) // e.g. 1.2, 2.0, ...
    {
        LOAD_GL_ARB(wglCreateContextAttribsARB);
        wglMakeCurrent(0,0);
        wglDeleteContext(m_context);

        // FIXME: recommend destroy window here

        int attribs[] =
        {
            WGL_CONTEXT_MAJOR_VERSION_ARB, in_major,
            WGL_CONTEXT_MINOR_VERSION_ARB, in_minor,
            WGL_CONTEXT_FLAGS_ARB,
            WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
            0
        };
        m_context = wglCreateContextAttribsARB(m_hDC, 0, attribs);
        success = wglMakeCurrent(m_hDC, m_context);
    }
}

//-----------------------------------------------------------------------------
// utility: load a shader & create appropriate shader type
//-----------------------------------------------------------------------------
inline GLuint GLContext::LoadShader(char* in_path, GLuint in_shaderType, std::string* out_pBuildLog)
{
    if (0 != out_pBuildLog)
    {
        out_pBuildLog->clear();
    }
    GLuint shaderID = 0;

    std::ifstream ifs(in_path);
    if (ifs.is_open())
    {
        std::string s((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        ifs.close();

        shaderID = glCreateShader(in_shaderType);
        const char* c_str = s.c_str();
        glShaderSource(shaderID, 1, &c_str, 0);
        glCompileShader(shaderID);
    }

    GLint success = GL_TRUE;
    GLint logLength;
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);
    if ((0 != out_pBuildLog) && (GL_TRUE != success))
    {

        std::string& buildLog = *out_pBuildLog;
	    glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &logLength);
        buildLog.resize(logLength);
	    glGetShaderInfoLog(shaderID, logLength, NULL, &buildLog[0]);
    }

    return shaderID;
}

//-----------------------------------------------------------------------------
// utility: load vertex and fragment shaders, return program
//-----------------------------------------------------------------------------
inline GLuint GLContext::LoadProgram(char* in_vertexPath, char* in_fragmentPath, std::string* out_pBuildLog)
{
    if (0 != out_pBuildLog)
    {
        out_pBuildLog->clear();
    }

    GLuint fragmentShaderID = LoadShader(in_fragmentPath, GL_FRAGMENT_SHADER, out_pBuildLog);
    if (out_pBuildLog && out_pBuildLog->size())
    {
        return 0;
    }
    GLuint vertexShaderID   = LoadShader(in_vertexPath,   GL_VERTEX_SHADER, out_pBuildLog);
    if (out_pBuildLog && out_pBuildLog->size())
    {
        return 0;
    }

    GLuint programID = 0;
    if (fragmentShaderID && vertexShaderID)
    {
        programID = glCreateProgram();
	    glAttachShader(programID, fragmentShaderID);
	    glAttachShader(programID, vertexShaderID);
	    glLinkProgram(programID);
        glDeleteShader(fragmentShaderID);
        glDeleteShader(vertexShaderID);

        GLint success = GL_TRUE;
        GLint logLength;
	    glGetProgramiv(programID, GL_LINK_STATUS, &success);
        if ((0 != out_pBuildLog) && (GL_TRUE != success))
        {
            std::string& buildLog = *out_pBuildLog;
 	        glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &logLength);
            buildLog.resize(logLength);
            glGetProgramInfoLog(programID, logLength, NULL, &buildLog[0]);
        }
    }
    return programID;
}

//-----------------------------------------------------------------------------
// handle window resize event
//-----------------------------------------------------------------------------
inline void GLContext::Resize()
{
    RECT rect;
    GetClientRect(m_hWnd, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    glViewport(0, 0, width, height);
}
