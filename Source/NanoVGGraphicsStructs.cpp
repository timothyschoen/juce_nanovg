#include "NanoVGGraphicsStructs.h"
#include "NanoVGGraphics.h"

APIBitmap::APIBitmap(NanoVGGraphics& g, int width_, int height_, float scale_, float drawScale_)
    : width(width_)
    , height(height_)
    , scale(scale_)
    , drawScale(drawScale_)
    , graphics(g)
    , FBO(nvgCreateFramebuffer(g.getContext(), width, height, 0))
{
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
    else
        drawCachable(g);

    drawAnimated(g);
}
