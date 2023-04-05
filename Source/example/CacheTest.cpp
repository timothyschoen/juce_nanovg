#include "CacheTest.h"


CacheTest::CacheTest(): NanoVGGraphics(*(juce::Component*)this)
{
    setCachedComponentImage(nullptr);
    setBufferedToImage(false);

    addAndMakeVisible(childComponent);

    startTimerHz(60);
}

CacheTest::~CacheTest()
{
}

void CacheTest::resized()
{
    bounds.R = (float)getWidth();
    bounds.B = (float)getHeight();

    const int x = getWidth() / 4;
    const int y = getHeight() / 4;

    childComponent.setBounds(x, y, x*2, y*2);
}

void CacheTest::paint(juce::Graphics &)
{
    render();
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

    childComponent.drawAnimated(g);
}

CacheTest::HoverTest::HoverTest()
{
}

void CacheTest::HoverTest::drawCachable(NanoVGGraphics&)
{
}

void CacheTest::HoverTest::drawAnimated(NanoVGGraphics& g)
{
    auto* nvg = g.getContext();

    if (mouseIsOver)
        nvgFillColor(nvg, nvgRGBA(200, 200, 100, 255));
    else
        nvgFillColor(nvg, nvgRGBA(100, 200, 200, 255));

    nvgBeginPath(nvg);
    nvgRect(nvg, (float)getX(), (float)getY(), (float)getWidth(), (float)getHeight());
    nvgFill(nvg);
}

void CacheTest::HoverTest::mouseEnter(const juce::MouseEvent &)
{
    mouseIsOver = true;
}

void CacheTest::HoverTest::mouseExit(const juce::MouseEvent &)
{
    mouseIsOver = false;
}

void CacheTest::HoverTest::resized()
{
    bounds = Rect(getBounds().toFloat());
}
