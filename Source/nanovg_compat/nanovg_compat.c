#include "nanovg_compat.h"
#include "nanovg.c"

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
