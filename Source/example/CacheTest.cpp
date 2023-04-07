#include "CacheTest.h"
#include <BinaryData.h>

CacheTest::CacheTest(): NanoVGGraphics(*(juce::Component*)this)
{
    useLayer = true;
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
    float y = std::floor(bounds.CY()) + 0.5f;
    nvgMoveTo(nvg, bounds.L, y);
    nvgLineTo(nvg, bounds.R, y);
    nvgStroke(nvg);

    childComponent.drawAnimated(g);
}

//==============================================================================

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

//==============================================================================

ScopedFramebufferTest::ScopedFramebufferTest(): NanoVGGraphics(*(juce::Component*)this)
{
    setCachedComponentImage(nullptr);
    setBufferedToImage(false);

    startTimerHz(60);
}

ScopedFramebufferTest::~ScopedFramebufferTest()
{
}

void ScopedFramebufferTest::contextCreated(NVGcontext* ctx)
{
    framebuffer.init(ctx);

    robotoFontId = nvgCreateFontMem(ctx, "robotoRegular", (unsigned char*)BinaryData::RobotoRegular_ttf, BinaryData::RobotoRegular_ttfSize, 0);
}

void ScopedFramebufferTest::paint(juce::Graphics&)
{
    render();
}

void ScopedFramebufferTest::draw(NanoVGGraphics& g)
{
    DBG("draw...");
    NVGcontext* ctx = g.getContext();

    nvgClearWithColor(ctx, nvgRGBAf(0.3f, 0.3f, 0.4f, 1.0f));

    if (!framebuffer.isValid())
    {
        Framebuffer::ScopedBind bind(g, framebuffer);

        nvgBeginPath(ctx);
        // framebuffers always begin at 0/0
        nvgRect(ctx, 0.0f, 0.0f, framebuffer.getWidth(), framebuffer.getHeight());
        nvgFillColor(ctx, nvgRGBAf(0.8f, 0.8f, 0.2f, 1.0f));
        nvgFill(ctx);
    }
    framebuffer.paint();

    nvgFontFaceId(ctx, robotoFontId);
    nvgFontSize(ctx, 24.0f);
    nvgTextAlign(ctx, NVG_ALIGN_CENTER|NVG_ALIGN_MIDDLE);
    nvgFillColor(ctx, nvgRGBAf(0.9f, 0.3f, 1.0f, 1.0f));
    nvgText(ctx, bounds.CX(), bounds.CY(), "< The rectangle to the left was cached!", nullptr);
}

void ScopedFramebufferTest::resized()
{
    bounds.R = (float)getWidth();
    bounds.B = (float)getHeight();

    framebuffer.setBounds((float)getWidth() * 0.05f, (float)getHeight() * 0.1f, (float)getWidth() * 0.33, (float)getHeight() * 0.8f);
}
