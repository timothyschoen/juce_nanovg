#ifndef NANOVG_COMPAT_H
#define NANOVG_COMPAT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <nanovg.h>

#ifdef _WIN32
#define D3D11_NO_HELPERS
#include <nanovg_d3d11.h>

struct D3DNVGframebuffer {
    ID3D11RenderTargetView* pRenderTargetView;
	int image; // nvg image id
};
typedef struct D3DNVGframebuffer D3DNVGframebuffer;

NVGcontext* d3dnvgCreateContext(HWND hwnd, int flags, unsigned  int width, unsigned int height);
HRESULT d3dnvgSetViewBounds(HWND hwnd, unsigned int width, unsigned int height);
void d3dnvgDeleteContext(NVGcontext* ctx);
void d3dnvgClearWithColor(NVGcontext* ctx, NVGcolor color);

// Binds the output-merger render target
void d3dnvgBindFramebuffer(NVGcontext* ctx, D3DNVGframebuffer* fb);
// Creates a 2D texture to use as a render target
D3DNVGframebuffer* d3dnvgCreateFramebuffer(NVGcontext* ctx, int w, int h, int flags);
// Deletes the frame buffer
void d3dnvgDeleteFramebuffer(NVGcontext* ctx, D3DNVGframebuffer* fb);

void d3dPresent(void);

#define nvgCreateContext(layer, flags, w, h) d3dnvgCreateContext((HWND)layer, flags, (unsigned)w, (unsigned)h)
#define nvgDeleteContext(context) d3dnvgDeleteContext(context)
#define nvgBindFramebuffer(ctx, fb) d3dnvgBindFramebuffer(ctx, fb)
#define nvgCreateFramebuffer(ctx, w, h, flags) d3dnvgCreateFramebuffer(ctx, w, h, flags)
#define nvgDeleteFramebuffer(ctx, fb) d3dnvgDeleteFramebuffer(ctx, fb)
#define nvgClearWithColor(ctx, color) d3dnvgClearWithColor(ctx, color)
#define nvgSetViewBounds(layer, w, h) d3dnvgSetViewBounds((HWND)layer, (unsigned)w, (unsigned)h)
typedef D3DNVGframebuffer NVGframebuffer;

#elif defined __APPLE__ 
#include <nanovg_mtl.h>

NVGcontext* mnvgCreateContext(void* view, int flags, int width, int height);
void mnvgSetViewBounds(void* view, int width, int height);

#define nvgCreateContext(layer, flags, w, h) mnvgCreateContext(layer, flags, w, h)
#define nvgDeleteContext(context) nvgDeleteMTL(context)
#define nvgBindFramebuffer(ctx, fb) mnvgBindFramebuffer(fb)
#define nvgCreateFramebuffer(ctx, w, h, flags) mnvgCreateFramebuffer(ctx, w, h, flags)
#define nvgDeleteFramebuffer(ctx, fb) mnvgDeleteFramebuffer(fb)
#define nvgClearWithColor(ctx, color) mnvgClearWithColor(ctx, color)
#define nvgSetViewBounds(nsview, w, h) mnvgSetViewBounds(nsview, w, h)
typedef MNVGframebuffer NVGframebuffer;

#elif defined __linux__
#include <nanovg_gl.h>
#include <nanovg_gl_utils.h>

#define nvgCreateContext(flags) nvgCreateGLES2(flags)
#define nvgDeleteContext(context) nvgDeleteGLES2(context)
#define nvgBindFramebuffer(ctx, fb) nvgluBindFramebuffer(fb)
#define nvgCreateFramebuffer(ctx, w, h, flags) nvgluCreateFramebuffer(ctx, w, h, flags)
#define nvgDeleteFramebuffer(ctx, fb) nvgluDeleteFramebuffer(fb)
typedef NVGLUframebuffer NVGframebuffer;

#endif


struct NanoVGDrawCallCount
{
    int draws;
    int fill;
    int stroke;
    int text;
    int total;
};
typedef struct NanoVGDrawCallCount NanoVGDrawCallCount;

NanoVGDrawCallCount nvgGetDrawCallCount(NVGcontext* ctx);

NVGcolor nvgGetFillColor(NVGcontext* ctx);

void nvgCurrentScissor(NVGcontext* ctx, float* x, float* y, float* w, float* h);


#ifdef __cplusplus
}
#endif

#endif  // NANOVG_COMPAT_H
