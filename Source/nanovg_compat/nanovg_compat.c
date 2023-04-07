#ifdef _WIN32
#define NANOVG_D3D11_IMPLEMENTATION
#elif defined __linux__
#define NANOVG_GLES2_IMPLEMENTATION
#endif

#include "nanovg_compat.h"

#ifdef __cplusplus
extern "C"{
#endif

#include "nanovg.c"

NanoVGDrawCallCount nvgGetDrawCallCount(NVGcontext* ctx)
{
    NanoVGDrawCallCount callCount;
    callCount.draws = ctx->drawCallCount;
    callCount.fill = ctx->fillTriCount;
    callCount.stroke = ctx->strokeTriCount;
    callCount.text = ctx->textTriCount;
    callCount.total = ctx->drawCallCount + ctx->fillTriCount + ctx->strokeTriCount + ctx->textTriCount;
    return callCount;
}

NVGcolor nvgGetFillColor(NVGcontext* ctx)
{
    NVGstate* state = nvg__getState(ctx);
    return state->fill.innerColor;
}

void nvgCurrentScissor(NVGcontext* ctx, float* x, float* y, float* w, float* h)
{
    NVGstate* state = nvg__getState(ctx);
    float pxform[6], invxorm[6];
    float ex, ey, tex, tey;

    // If no previous scissor has been set, set the scissor as current scissor.
    if (state->scissor.extent[0] < 0) {
        return;
    }

    // Transform the current scissor rect into current transform space.
    // If there is difference in rotation, this will be approximation.
    memcpy(pxform, state->scissor.xform, sizeof(float)*6);
    ex = state->scissor.extent[0];
    ey = state->scissor.extent[1];
    nvgTransformInverse(invxorm, state->xform);
    nvgTransformMultiply(pxform, invxorm);
    tex = ex*nvg__absf(pxform[0]) + ey*nvg__absf(pxform[2]);
    tey = ex*nvg__absf(pxform[1]) + ey*nvg__absf(pxform[3]);

    *x = pxform[4]-tex;
    *y = pxform[5]-tey;
    *w = tex*2;
    *h = tey*2;
}

#ifdef NANOVG_D3D11_IMPLEMENTATION

#define WCODE_HRESULT_FIRST MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x200)
#define WCODE_HRESULT_LAST MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF+1, 0) - 1
#define HRESULTToWCode(r) (r >= WCODE_HRESULT_FIRST && r <= WCODE_HRESULT_LAST) ? (r - WCODE_HRESULT_FIRST) : 0


ID3D11Device* pDevice = NULL;
ID3D11DeviceContext* pDeviceContext = NULL;
IDXGISwapChain* pSwapChain = NULL;
DXGI_SWAP_CHAIN_DESC swapDesc;
ID3D11RenderTargetView* pRenderTargetView;
ID3D11Texture2D* pDepthStencil;
ID3D11DepthStencilView* pDepthStencilView;
ID3D11RenderTargetView* D3D__currentlyBoundView = NULL;

void d3dPresent(void)
{
    D3D_API_2(pSwapChain, Present, 0, 0);
}

// Frees everything
static void D3D__shutdownDX11()
{
    // Detach RTs
    if (pDeviceContext)
    {
        ID3D11RenderTargetView *viewList[1] = { NULL };
        D3D_API_3(pDeviceContext, OMSetRenderTargets, 1, viewList, NULL);
    }

    D3D_API_RELEASE(pDeviceContext);
    D3D_API_RELEASE(pDevice);
    D3D_API_RELEASE(pSwapChain);
    D3D_API_RELEASE(pRenderTargetView);
    D3D_API_RELEASE(pDepthStencil);
    D3D_API_RELEASE(pDepthStencilView);
}

// Setup the device and the rendering targets
static HRESULT D3D__initializeDX11(HWND hwnd, unsigned int width, unsigned int height)
{
    HRESULT hr = S_OK;
    IDXGIDevice *pDXGIDevice = NULL;
    IDXGIAdapter *pAdapter = NULL;
    IDXGIFactory *pDXGIFactory = NULL;
    UINT deviceFlags = 0;
    UINT driver = 0;

    static const D3D_DRIVER_TYPE driverAttempts[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };

    static const D3D_FEATURE_LEVEL levelAttempts[] =
    {
        D3D_FEATURE_LEVEL_11_0,  // Direct3D 11.0 SM 5
        D3D_FEATURE_LEVEL_10_1,  // Direct3D 10.1 SM 4
        D3D_FEATURE_LEVEL_10_0,  // Direct3D 10.0 SM 4
        D3D_FEATURE_LEVEL_9_3,   // Direct3D 9.3  SM 3
        D3D_FEATURE_LEVEL_9_2,   // Direct3D 9.2  SM 2
        D3D_FEATURE_LEVEL_9_1,   // Direct3D 9.1  SM 2
    };

    for (driver = 0; driver < ARRAYSIZE(driverAttempts); driver++)
    {
        hr = D3D11CreateDevice(
            NULL,
            driverAttempts[driver],
            NULL,
            deviceFlags,
            levelAttempts,
            ARRAYSIZE(levelAttempts),
            D3D11_SDK_VERSION,
            &pDevice,
            NULL,
            &pDeviceContext
            );

        if (SUCCEEDED(hr))
        {
            char text[32];
            ZeroMemory(text, sizeof(text));
            sprintf(text, "Using graphics driver: %d\n", driver);
            OutputDebugString(text);

            break;
        }
    }

    if (FAILED(hr))
    {
        OutputDebugString("Failed D3D11CreateDevice()\n");
    }

    if (SUCCEEDED(hr))
    {
        hr = D3D_API_2(pDevice, QueryInterface, &IID_IDXGIDevice, (void**)&pDXGIDevice);
        if (FAILED(hr))
        {
            OutputDebugString("Failed ID3D11Device::QueryInterface()\n");
        }
    }
    if (SUCCEEDED(hr))
    {
        hr = D3D_API_1(pDXGIDevice, GetAdapter, &pAdapter);
        if (FAILED(hr))
        {
            OutputDebugString("Failed IDXGIDevice::GetAdapter()\n");
        }
    }
    if (SUCCEEDED(hr))
    {
        hr = D3D_API_2(pAdapter, GetParent, &IID_IDXGIFactory, (void**)&pDXGIFactory);
        if (FAILED(hr))
        {
            OutputDebugString("Failed IDXGIAdapter::GetParent()\n");
        }
    }
    if (SUCCEEDED(hr))
    {
        ZeroMemory(&swapDesc, sizeof(swapDesc));

        swapDesc.SampleDesc.Count = 1;        //The Number of Multisamples per Level
        swapDesc.SampleDesc.Quality = 0;      //between 0(lowest Quality) and one lesser than pDevice->CheckMultisampleQualityLevels

        swapDesc.OutputWindow = hwnd;
        swapDesc.Windowed = TRUE;
        swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapDesc.BufferDesc.Width = width;
        swapDesc.BufferDesc.Height = height;
        swapDesc.BufferDesc.RefreshRate.Numerator = 60;
        swapDesc.BufferDesc.RefreshRate.Denominator = 1;
        swapDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        swapDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

        // According to randos the internet, BGRA is a format favoured by most hardware.
        // Using RGBA will often mean that each pixel will have to be converted (swizzled),
        // which apprently has a ~5% performance cost.
        swapDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;

        // Using a flip discard model is faster and recommended by Microsoft, but the feature is only
        // available starting from Windows 8.
        // Recommended settings: https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-flip-model
        // First we will attempt the recommended model
        swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapDesc.BufferCount = 2;
        hr = D3D_API_3(pDXGIFactory, CreateSwapChain, (IUnknown*)pDevice, &swapDesc, &pSwapChain);

        if (FAILED(hr))
        {
            OutputDebugString("Failed to CreateSwapChain using DXGI_SWAP_EFFECT_FLIP_DISCARD strategy, using fallback...\n");
            // These settings should work on Windows 7...
            swapDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
            swapDesc.BufferCount = 1;
            hr = D3D_API_3(pDXGIFactory, CreateSwapChain, (IUnknown*)pDevice, &swapDesc, &pSwapChain);
            if (FAILED(hr))
            {
                OutputDebugString("Failed IDXGIFactory::CreateSwapChain()\n");
            }
        }
    }

    D3D_API_RELEASE(pDXGIDevice);
    D3D_API_RELEASE(pAdapter);
    D3D_API_RELEASE(pDXGIFactory);

    if (SUCCEEDED(hr))
    {
        hr = d3dnvgSetViewBounds(hwnd, width, height);
        if (FAILED(hr))
        {
            OutputDebugString("Failed to set view bounds - d3dnvgSetViewBounds()\n");
            return hr;
        }
    }
    else
    {
        // Fail
        return hr;
    }

    return hr;
}

NVGcontext* d3dnvgCreateContext(HWND hwnd, int flags, unsigned int width, unsigned int height)
{
    NVGcontext* ctx = NULL;
    HRESULT hr = D3D__initializeDX11(hwnd, width, height);
    if (FAILED(hr))
    {
        OutputDebugString("Could not Initialize DX11\n");
        return NULL;
    }
    OutputDebugString("Initialized DX11\n");

    ctx = nvgCreateD3D11(pDevice, flags);
    if (ctx == NULL)
    {
        OutputDebugString("Failed creating NVGcontext\n");
        return NULL;
    }
    return ctx;
}

void d3dnvgDeleteContext(NVGcontext* ctx)
{
    D3D__shutdownDX11();
    nvgDeleteD3D11(ctx);
}

HRESULT d3dnvgSetViewBounds(HWND hwnd, unsigned int width, unsigned int height)
{
    D3D11_RENDER_TARGET_VIEW_DESC renderDesc;
    ID3D11RenderTargetView *viewList[1] = { NULL };
    HRESULT hr = S_OK;
    ID3D11Resource *pBackBufferResource = NULL;
    D3D11_TEXTURE2D_DESC texDesc;
    D3D11_DEPTH_STENCIL_VIEW_DESC depthViewDesc;

    swapDesc.BufferDesc.Width = width;
    swapDesc.BufferDesc.Height = height;

    if (!pDevice || !pDeviceContext)
    {
        OutputDebugString("Cannot use d3dnvgSetViewBounds(). ID3D11Device & ID3D11DeviceContext not initialised.\n");
        return E_FAIL;
    }

    //pDeviceContext->ClearState();
    D3D_API_3(pDeviceContext, OMSetRenderTargets, 1, viewList, NULL);

    // Ensure that nobody is holding onto one of the old resources
    D3D_API_RELEASE(pRenderTargetView);
    D3D_API_RELEASE(pDepthStencilView);

    // Resize render target buffers
    hr = D3D_API_5(pSwapChain, ResizeBuffers, swapDesc.BufferCount, width, height, swapDesc.BufferDesc.Format, 0);
    if (FAILED(hr))
    {
        OutputDebugString("Failed IDXGISwapChain::ResizeBuffers()\n");
        return hr;
    }

    // Create the render target view and set it on the device
    hr = D3D_API_3(pSwapChain, GetBuffer, 0, &IID_ID3D11Texture2D, (void**)&pBackBufferResource);
    if (FAILED(hr))
    {
        OutputDebugString("Failed IDXGISwapChain::GetBuffer()\n");
        return hr;
    }

    renderDesc.Format = swapDesc.BufferDesc.Format;
    renderDesc.ViewDimension = (swapDesc.SampleDesc.Count>1) ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D;
    renderDesc.Texture2D.MipSlice = 0;

    hr = D3D_API_3(pDevice, CreateRenderTargetView, pBackBufferResource, &renderDesc, &pRenderTargetView);
    D3D_API_RELEASE(pBackBufferResource);
    if (FAILED(hr))
    {
        OutputDebugString("Failed ID3D11Device::CreateRenderTargetView()\n");
        return hr;
    }

    texDesc.ArraySize = 1;
    texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    texDesc.CPUAccessFlags = 0;
    texDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    texDesc.Height = (UINT)height;
    texDesc.Width = (UINT)width;
    texDesc.MipLevels = 1;
    texDesc.MiscFlags = 0;
    texDesc.SampleDesc.Count = swapDesc.SampleDesc.Count;
    texDesc.SampleDesc.Quality = swapDesc.SampleDesc.Quality;
    texDesc.Usage = D3D11_USAGE_DEFAULT;

    D3D_API_RELEASE(pDepthStencil);
    hr = D3D_API_3(pDevice, CreateTexture2D, &texDesc, NULL, &pDepthStencil);
    if (FAILED(hr))
    {
        OutputDebugString("Failed ID3D11Device::CreateTexture2D()\n");
        return hr;
    }

    depthViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthViewDesc.ViewDimension = (swapDesc.SampleDesc.Count>1) ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;
    depthViewDesc.Flags = 0;
    depthViewDesc.Texture2D.MipSlice = 0;

    hr = D3D_API_3(pDevice, CreateDepthStencilView, (ID3D11Resource*)pDepthStencil, &depthViewDesc, &pDepthStencilView);
    if (FAILED(hr))
    {
        OutputDebugString("Failed ID3D11Device::CreateDepthStencilView()\n");
        return hr;
    }
    return hr;
}

void d3dnvgClearWithColor(NVGcontext* ctx, NVGcolor color)
{
    D3D_API_2(pDeviceContext, ClearRenderTargetView, D3D__currentlyBoundView, color.rgba);
    D3D_API_4(pDeviceContext, ClearDepthStencilView, pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 0.0f, 0u);
}

void d3dnvgBindFramebuffer(D3DNVGframebuffer* fb)
{
    D3D11_VIEWPORT viewport;
    ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

	if (fb == NULL)
	{
        viewport.Width = swapDesc.BufferDesc.Width;
        viewport.Height = swapDesc.BufferDesc.Height;
        D3D__currentlyBoundView = pRenderTargetView;
    }
    else
    {
        viewport.Width = fb->width;
        viewport.Height = fb->height;
        D3D__currentlyBoundView = fb->pRenderTargetView;
    }

    D3D_API_3(pDeviceContext, OMSetRenderTargets, 1, &D3D__currentlyBoundView, pDepthStencilView);
    D3D_API_2(pDeviceContext, RSSetViewports, 1, &viewport);
}

D3DNVGframebuffer* d3dnvgCreateFramebuffer(NVGcontext* ctx, int w, int h, int flags)
{
    HRESULT hr = S_OK;

    D3DNVGframebuffer* fb = (D3DNVGframebuffer*)malloc(sizeof(D3DNVGframebuffer));
    if (fb == NULL)
    {
        OutputDebugString("Failed to allocate a framebuffer");
        return NULL;
    }

    ZeroMemory(fb, sizeof(D3DNVGframebuffer));
    fb->image = nvgCreateImageRGBA(ctx, w, h, flags | NVG_IMAGE_RENDER_TARGET, NULL);
    fb->width = w;
    fb->height = h;

    NVGparams* params = nvgInternalParams(ctx);
    struct D3DNVGcontext* D3D = (struct D3DNVGcontext*)params->userPtr;
    struct D3DNVGtexture* tex = D3Dnvg__findTexture(D3D, fb->image);

    D3D11_RENDER_TARGET_VIEW_DESC renderDesc;
    ZeroMemory(&renderDesc, sizeof(D3D11_RENDER_TARGET_VIEW_DESC));
    // nanovg images use the RGBA format, so we must use the same else the red & blue channels will be flipped
    renderDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    renderDesc.ViewDimension = (swapDesc.SampleDesc.Count>1) ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D;
    renderDesc.Texture2D.MipSlice = 0;

	hr = D3D_API_3(pDevice, CreateRenderTargetView, tex->tex, &renderDesc, &fb->pRenderTargetView);
    if (FAILED(hr))
	{
		WORD code = HRESULTToWCode(hr);
		OutputDebugString("Failed creating frame buffer ID3D11Device::CreateRenderTargetView()");
        return NULL;
	}

    return fb;
}

void d3dnvgDeleteFramebuffer(NVGcontext* ctx, D3DNVGframebuffer* fb)
{
    if (fb == NULL) return;
    if (fb->image > 0)
    {
        nvgDeleteImage(ctx, fb->image);
    }
    D3D_API_RELEASE(fb->pRenderTargetView);
    free(fb);
}

#ifdef __cplusplus
}
#endif

#endif
