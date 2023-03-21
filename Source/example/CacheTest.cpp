#include "CacheTest.h"


CacheTest::CacheTest(): NanoVGGraphics(*(juce::Component*)this)
{
    setOpaque(true);
    setCachedComponentImage(nullptr);
    setBufferedToImage(false);
    startTimerHz(1);
}

CacheTest::~CacheTest()
{
}

void CacheTest::resized()
{
    bounds.R = getWidth();
    bounds.B = getHeight();
}

void CacheTest::drawCachable(NanoVGGraphics& g)
{
    DBG("drawCachable");
    auto* nvg = g.getContext();
    nvgFillColor(nvg, nvgRGBA(127, 127, 127, 255));
    nvgBeginPath(nvg);
    nvgRect(nvg, bounds.L, bounds.T, bounds.W(), bounds.H());
    nvgFill(nvg);
}

void CacheTest::drawAnimated(NanoVGGraphics& g)
{
    DBG("drawAnimated");
    auto* nvg = g.getContext();
    nvgStrokeColor(nvg, nvgRGBA(255,255,255,128));
    nvgStrokeWidth(nvg, 1.0f);
    nvgBeginPath(nvg);
    nvgMoveTo(nvg, bounds.L, bounds.CY());
    nvgLineTo(nvg, bounds.R, bounds.CY());
    nvgStroke(nvg);
}