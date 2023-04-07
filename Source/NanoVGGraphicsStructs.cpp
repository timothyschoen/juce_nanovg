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

//==============================================================================

Framebuffer::~Framebuffer()
{
    if (fb)
        nvgDeleteFramebuffer(ctx, fb);
}

void Framebuffer::init(NVGcontext* context)
{
    ctx = context;

    if (width > 0 && height > 0)
        fb = nvgCreateFramebuffer(ctx, width, height, 0);
}

void Framebuffer::paint()
{
    jassert(ctx != nullptr);
    jassert(fb != nullptr);

    NVGpaint paint= nvgImagePattern(ctx, x, y, width, height, 0.0f, fb->image, 1.0f);
    nvgBeginPath(ctx); // clears existing path cache
    nvgRect(ctx, x, y, width, height); // fills the path with a rectangle
    nvgFillPaint(ctx, paint); // sets the paint settings to the current state
    nvgFill(ctx); // draw call happens here, using the paint settings
}

void Framebuffer::setBounds(float x_, float y_, float width_, float height_)
{
    x = x_;
    y = y_;
    width = width_;
    height = height_;

    // invalidate
    valid = false;

    // Unfornately there is no way to simply resize the existing texture...
    // https://github.com/Microsoft/DirectXTK/issues/93
    if (fb)
        nvgDeleteFramebuffer(ctx, fb);

    if (ctx)
        fb = nvgCreateFramebuffer(ctx, width, height, 0);
}

//==============================================================================

Framebuffer::ScopedBind::ScopedBind(NanoVGGraphics& g, Framebuffer& f)
    : graphics(g)
    , fb(f)
{
    DBG("binding texture");
    auto* ctx = graphics.getContext();
    nvgEndFrame(ctx);
    nvgBindFramebuffer(fb.get());
    nvgBeginFrame(ctx, fb.getWidth(), fb.getHeight(), 1.0f);
}

Framebuffer::ScopedBind::~ScopedBind()
{
    DBG("releasing bind");
    auto* ctx = graphics.getContext();
    nvgEndFrame(ctx);
    fb.setValid(true);

    nvgBindFramebuffer(graphics.getMainFramebuffer());
    nvgBeginFrame(ctx, graphics.getWindowWidth(), graphics.getWindowHeight(), 1.0f);
}
