#include "NanoVGGraphicsStructs.h"
#include <juce_core/juce_core.h>

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
