/*
Copyright (c) 2012, Intel Corporation

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <Windows.h>
#include <stdio.h>
#include <time.h>

#include "display/SampleWindow.h"
#include "SimulateCL.h"

#include "display/GLContext.h"
#include "RenderGL.h"

#define NUM_POINTS     1024*64
#define RANGE          1000.0f
// initial speed: distance that can be covered in 1 time tick:
#define SPEED          (RANGE*.001f)

#define TRANSLATION_RATE  0.3f
#define TRANSLATION_TURBO 10
#define ROTATION_RATE     0.01f

// target refresh rate of 60Hz.
// scale from seconds.
const float SIM_STEP_INCREMENT = 0.01f / (1.0f / 60.0f);
float SIM_STEP_SCALE = 4*SIM_STEP_INCREMENT;

const float MIN_SIM_SCALE = -10 * SIM_STEP_INCREMENT;
const float MAX_SIM_SCALE = 10 * SIM_STEP_INCREMENT;

const float MAX_TIME_DELTA_SECONDS = 50.0f/1000.0f; // maximum time step in seconds

//-----------------------------------------------------------------------------
// window class
//-----------------------------------------------------------------------------
class MyWindow : public SampleWindow
{
public:
    virtual bool WndProc(HWND, UINT, WPARAM, LPARAM);
    virtual void MainLoop();
    virtual void Init(HWND in_hWnd);
    virtual void Shutdown();

    MyWindow();
private:
    void KeyHandler();
    bool m_hasFocus;

    Vector2i m_mousePos;
    Vector2i m_mouseLeftDelta;
    Vector2i m_mouseRightDelta;
    Vector2i m_keyTranslation;

    GLContext m_gfxContext;
    SimulateCL m_SimulateCL;
    RenderGL m_render;
};

//-----------------------------------------------------------------------------
// constructor
//-----------------------------------------------------------------------------
MyWindow::MyWindow()
{
    m_keyTranslation = Vector2i(0,0);
    m_mouseLeftDelta = Vector2i(0,0);
    m_mouseRightDelta = Vector2i(0,0);
    m_hasFocus = true; // application starts with focus
}

//-----------------------------------------------------------------------------
// key presses -> translation to render loop
//-----------------------------------------------------------------------------
void MyWindow::KeyHandler()
{
    if (m_hasFocus)
    {
        // Keyboard Translation
        SHORT forward = (GetKeyState('W') | GetKeyState(VK_UP)) >> 8;
        SHORT back    = (GetKeyState('S') | GetKeyState(VK_DOWN)) >> 8;
        SHORT left    = (GetKeyState('A') | GetKeyState(VK_LEFT)) >> 8;
        SHORT right   = (GetKeyState('D') | GetKeyState(VK_RIGHT)) >> 8;

        SHORT sSHIFTkey = GetKeyState(VK_SHIFT) >> 8;

        Vector2i translation( (left?-1:0) + (right?1:0), (forward?1:0)+(back?-1:0));

        if (sSHIFTkey)
        {
            translation *= TRANSLATION_TURBO;
        }

        m_keyTranslation = translation;
    }
}

//-----------------------------------------------------------------------------
// windows message handler
//-----------------------------------------------------------------------------
bool MyWindow::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch( message )
    {
    case WM_SETFOCUS:
        m_hasFocus = true;
        break;
    case WM_KILLFOCUS:
        m_hasFocus = false;
        break;

    case WM_SIZE:
        m_gfxContext.Resize();
        m_render.Resize(hWnd);
        break;

    case WM_MOUSEMOVE:
        {
            int x = lParam & 0x0000ffff;
            int y = lParam >> 16;
            Vector2i pos(x, y);
            if (wParam & MK_LBUTTON)
            {
                m_mouseLeftDelta += pos - m_mousePos;
            }
            if (wParam & MK_RBUTTON)
            {
                m_mouseRightDelta += pos - m_mousePos;
            }
            m_mousePos = pos;
        }
        break;

    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_F1:
            MessageBox(hWnd,
                "Particle positions are animated in OpenCL and rendered with OpenGL.\n"
                "Keys:\n"
                "\tMouse left button + drag for local rotation\n"
                "\tMouse right button + drag for object rotation\n"
                "\tArrow keys or W, A, S, D for movement\n"
                "\tHold Shift to move faster\n",
                "OpenCL / OpenGL Buffer Sharing", MB_OK);
            break;
        case VK_ADD:
            if (SIM_STEP_SCALE < MAX_SIM_SCALE) SIM_STEP_SCALE += SIM_STEP_INCREMENT;
            break;
        case VK_SUBTRACT:
            if (SIM_STEP_SCALE > MIN_SIM_SCALE) SIM_STEP_SCALE -= SIM_STEP_INCREMENT;
            break;
        }
        break;

    default:
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
// get time since last update.
// tries to throttle to close to refresh rate of ~60Hz
//-----------------------------------------------------------------------------
float GetTimeStep()
{
    static clock_t prev = clock();
    clock_t minDelta = 16 * (CLOCKS_PER_SEC/1000); // 16ms is approximately 60Hz

    clock_t c = clock();
    clock_t delta = c - prev;
    if (delta < minDelta)
    {
        Sleep(10); // suspend thread for at least 10ms.
        clock_t c = clock();
        delta = c - prev;
    }
    prev = c;

    float step = float(delta) / float(CLOCKS_PER_SEC); // step in seconds
    if (step > MAX_TIME_DELTA_SECONDS)
    {
        step = MAX_TIME_DELTA_SECONDS;
    }

    step *= SIM_STEP_SCALE;

    return step;
}

//-----------------------------------------------------------------------------
// render loop
//-----------------------------------------------------------------------------
void MyWindow::MainLoop()
{
    KeyHandler();

    m_gfxContext.Flush(); // FIXME: required?

    float step = GetTimeStep();

    m_SimulateCL.Step(step);

    Vector2s rotation((float)m_mouseLeftDelta.x, (float)m_mouseLeftDelta.y);
    Vector2s translation((float)m_keyTranslation.x, (float)m_keyTranslation.y);
    Vector2s objRotation((float)m_mouseRightDelta.x, (float)m_mouseRightDelta.y);

    m_render.Render(
        rotation * ROTATION_RATE,
        translation * TRANSLATION_RATE,
        objRotation * ROTATION_RATE);

    m_gfxContext.Present();

    m_mouseLeftDelta = Vector2i(0,0); // reset delta, which has been consumed
    m_mouseRightDelta = Vector2i(0,0); // reset delta, which has been consumed
}

//-----------------------------------------------------------------------------
// initialize windows message handler
//-----------------------------------------------------------------------------
void MyWindow::Init(HWND in_hWnd)
{
    // start graphics context
    m_gfxContext.Init(in_hWnd);

    // start scene rendering object and create shared buffers
    GLuint points;
    m_render.Init(NUM_POINTS, &points);

    // start OCL & share buffers
    HGLRC pRC = 0;
    HDC   pDC = 0;
    m_gfxContext.GetRC(&pRC, &pDC);
    cl_int status = m_SimulateCL.Init(pRC, pDC, NUM_POINTS, points, RANGE, SPEED);

    m_render.Resize(in_hWnd);
}

//-----------------------------------------------------------------------------
// take things down in an orderly fashion (reverse of initialization)
//-----------------------------------------------------------------------------
void MyWindow::Shutdown()
{
    m_SimulateCL.Shutdown();
    m_render.Shutdown();
    m_gfxContext.Shutdown();
}

//-----------------------------------------------------------------------------
// Create window and start message pump
//-----------------------------------------------------------------------------
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    MyWindow m;
    return m.Show(hInstance, nCmdShow, "OpenCL Particles rendered with OpenGL");
}
