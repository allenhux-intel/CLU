/*
Copyright (c) 2012, Intel Corporation

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#pragma once
#include <Windows.h>

//=============================================================================
// Howto:
//   1. Derive a class from SampleWindow that implements the virtual members
//   2. instantiate this class from your wWinMain:
//
//      int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
//                          LPWSTR lpCmdLine, int nCmdShow)
//      {
//           MyWindow m();
//           return m.Show(hInstance, nCmdShow, "My Caption");
//       }
//=============================================================================
class SampleWindow
{
public:
    // show the window:
    int Show(HINSTANCE in_hInstance, int in_nCmdShow, char* in_pWindowName);
protected:
    // called to handle windows messages, e.g. WM_FOO
    // return "false" if it did NOT handle the message (so we can call DefWindowProc)
    virtual bool WndProc(HWND, UINT, WPARAM, LPARAM) = 0;

    // render loop. FIXME: could also pass some, but not all, WM messages here
    virtual void MainLoop() = 0;

    // called once at startup
    virtual void Init(HWND in_hWnd) = 0;

    // called once before PostQuitMessage()
    virtual void Shutdown() = 0;
private:
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    static const int WINDOW_WIDTH = 1024;
    static const int WINDOW_HEIGHT = 768;
    HINSTANCE m_hInst;
    HWND      m_hWnd;
};

//-----------------------------------------------------------------------------
// Windows message handler. Calls application-provided message handler
//-----------------------------------------------------------------------------
inline LRESULT CALLBACK SampleWindow::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    LONG_PTR pUserData = GetWindowLongPtr(hWnd, GWLP_USERDATA);
    SampleWindow* pCallbacks = reinterpret_cast<SampleWindow*>(pUserData);
    if (pCallbacks)
    {
        bool Handled = pCallbacks->WndProc(hWnd, message, wParam, lParam);
    }

    switch(message)
    {
        case WM_PAINT:
            hdc = BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
            break;

        case WM_DESTROY:
            if (pCallbacks) {pCallbacks->Shutdown();}
            PostQuitMessage(0);
            break;

		case WM_CHAR:
			if (wParam==VK_ESCAPE)
            {
                if (pCallbacks) {pCallbacks->Shutdown();}
				PostQuitMessage(0);
            }
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}


#define IDI_SAMPLE_ICON 101

//-----------------------------------------------------------------------------
// Create window and start windows message pump
//-----------------------------------------------------------------------------
inline int SampleWindow::Show(HINSTANCE in_hInstance, int in_nCmdShow, char* in_pWindowName)
{
    m_hInst = in_hInstance;

    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC; // note: CS_OWNDC required for GL
    wcex.lpfnWndProc = WindowProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = in_hInstance;
    wcex.hIcon = LoadIcon(in_hInstance, (LPCTSTR)IDI_SAMPLE_ICON);
    wcex.hCursor = LoadCursor(0, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = 0;
    wcex.lpszClassName = "WindowClass";
    wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SAMPLE_ICON);

    if(!RegisterClassEx(&wcex))
    {
        return E_FAIL;
    }

    // Create window
    RECT rc = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    m_hWnd = CreateWindow(
        "WindowClass",
        in_pWindowName,
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top, 0, 0, in_hInstance, 0);

    if(!m_hWnd)
    {
        return E_FAIL;
    }

    ShowWindow(m_hWnd, in_nCmdShow);

    // set user data of window to be the class from the calling application
    SetWindowLongPtr(m_hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    // initialize callback class with newly created window handle:
    Init(m_hWnd);

    // Main message loop
    MSG msg = {0};
    while( WM_QUIT != msg.message )
    {
        if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        else
        {
            MainLoop();
        }
    }

    return (int)msg.wParam;
}
