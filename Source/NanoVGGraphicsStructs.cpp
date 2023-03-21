#include "NanoVGGraphicsStructs.h"
#include "NanoVGGraphics.h"

APIBitmap::APIBitmap(NanoVGGraphics& g, int width_, int height_, float scale_, float drawScale_)
    : width(width_)
    , height(height_)
    , scale(scale_)
    , drawScale(drawScale_)
    , graphics(g)
{
    auto* nvg = g.getContext();
    FBO = mnvgCreateFramebuffer(nvg, width, height, 0);
    mnvgBindFramebuffer(FBO);
    mnvgClearWithColor(nvg, nvgRGBA(0, 0, 0, 0));

    nvgBeginFrame(nvg, width, height, 1.0f);
    nvgEndFrame(nvg);
}

APIBitmap::~APIBitmap()
{
    if(FBO)
        graphics.deleteFBO(FBO);
}

Layer::Layer(APIBitmap* bmp, const Rect& layerRect, ComponentLayer* comp)
    : bitmap(bmp)
    , component(comp)
    , componentRect(comp->bounds)
    , rect(layerRect)
    , invalid(false)
{
}

void ComponentLayer::draw(NanoVGGraphics& g)
{
    jassert(!bounds.isEmpty());
    if(useLayer)
    {
        if (!g.checkLayer(layer))
        {
            g.startLayer(this, bounds);
            drawCachable(g);
            layer = g.endLayer();
        }

        g.drawLayer(layer, &blend);
    }
    
    drawAnimated(g);
}
