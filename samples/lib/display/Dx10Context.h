/*
Copyright (c) 2012, Intel Corporation

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#pragma once

#include <D3D10.h>
#include <assert.h>

class Dx10Context
{
public:
    Dx10Context();
    ~Dx10Context();
    HRESULT Init(HWND in_hWnd);
    void Resize();
    void Flush()                                    {GetDevice()->Flush();}
    HRESULT Present()                               {return m_pSwapChain->Present(0,0);}
    ID3D10Device* GetDevice()                       {return m_pDevice;}
    ID3D10RenderTargetView*   GetRenderTargetView() {return m_pRenderTargetView;}

    void Shutdown();
private:
    D3D10_DRIVER_TYPE       m_driverType;
    IDXGISwapChain*         m_pSwapChain;
    ID3D10Device*           m_pDevice;
    ID3D10RenderTargetView* m_pRenderTargetView;
    HWND                    m_hWnd;

    void CreateRTV(int in_width, int in_height);
};

#define SAFE_RELEASE(x) if (x) {x->Release(); x=0;}

//-----------------------------------------------------------------------------
// constructor
//-----------------------------------------------------------------------------
inline Dx10Context::Dx10Context()
{
    m_pSwapChain = 0;
    m_pDevice = 0;
    m_pRenderTargetView = 0;
}

//-----------------------------------------------------------------------------
// destructor
//-----------------------------------------------------------------------------
inline Dx10Context::~Dx10Context()
{
    assert(0 == m_pDevice);
}


//-----------------------------------------------------------------------------
// recommend calling Shutdown the window
//-----------------------------------------------------------------------------
inline void Dx10Context::Shutdown()
{
	if (m_pDevice)
    {
        m_pDevice->OMSetRenderTargets( 0, NULL, NULL );
    }

    SAFE_RELEASE(m_pRenderTargetView);
    SAFE_RELEASE(m_pDevice);
    SAFE_RELEASE(m_pSwapChain);
}

//-----------------------------------------------------------------------------
// initialize DX context from HWND
//-----------------------------------------------------------------------------
inline HRESULT Dx10Context::Init(HWND in_hWnd)
{
    m_hWnd = in_hWnd;

    HRESULT hr = S_OK;

    RECT rc;
    GetClientRect( in_hWnd, &rc );
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    UINT createDeviceFlags = 0;
#if defined( DEBUG ) || defined( _DEBUG )
    createDeviceFlags |= D3D10_CREATE_DEVICE_DEBUG;
#endif

    D3D10_DRIVER_TYPE driverTypes[] =
    {
        D3D10_DRIVER_TYPE_HARDWARE,
        D3D10_DRIVER_TYPE_WARP,
        D3D10_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE( driverTypes );

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
	UINT numFeatureLevels = ARRAYSIZE( featureLevels );

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory( &sd, sizeof( sd ) );
    sd.BufferCount = 1;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = in_hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

    for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
    {
        m_driverType = driverTypes[driverTypeIndex];
        hr = D3D10CreateDeviceAndSwapChain( NULL, m_driverType, NULL, createDeviceFlags, D3D10_SDK_VERSION, &sd,
                                            &m_pSwapChain, &m_pDevice );
        if( SUCCEEDED( hr ) )
            break;
    }
    if( FAILED( hr ) )
        return hr;

    CreateRTV(width, height);

    return hr;
}

//-----------------------------------------------------------------------------
// internal routine to create the render target view
//-----------------------------------------------------------------------------
inline void Dx10Context::CreateRTV(int in_width, int in_height)
{
    HRESULT hr = S_OK;

    // Create a render target view
    ID3D10Texture2D* pBackBuffer = NULL;
    hr = m_pSwapChain->GetBuffer( 0, __uuidof( ID3D10Texture2D ), ( LPVOID* )&pBackBuffer );

	hr = m_pDevice->CreateRenderTargetView( pBackBuffer, NULL, &m_pRenderTargetView );
    pBackBuffer->Release();
	m_pDevice->OMSetRenderTargets( 1, &m_pRenderTargetView, NULL );

    // Setup the viewport
    D3D10_VIEWPORT vp;
    vp.Width = in_width;
    vp.Height = in_height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    m_pDevice->RSSetViewports( 1, &vp );
}

//-----------------------------------------------------------------------------
// resize render target on window resize
//-----------------------------------------------------------------------------
inline void Dx10Context::Resize()
{
    SAFE_RELEASE(m_pRenderTargetView);
    RECT rect;
    GetClientRect(m_hWnd, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    m_pSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
    CreateRTV(width, height);
}
