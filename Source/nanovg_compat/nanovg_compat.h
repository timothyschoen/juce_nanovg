#ifndef NANOVG_COMPAT_H
#define NANOVG_COMPAT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "nanovg.h"


void nvgCurrentScissor(NVGcontext* ctx, float* x, float* y, float* w, float* h);

#ifdef NANOVG_METAL_IMPLEMENTATION
#include "nanovg_mtl.h"

NVGcontext* mnvgCreateContext(void* view, int flags, int width, int height);
void mnvgSetViewBounds(void* view, int width, int height);
#endif

#ifdef __cplusplus
}
#endif

#endif  // NANOVG_COMPAT_H
