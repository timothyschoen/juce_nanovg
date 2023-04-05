#include "NanoVGGraphics.h"


static void setNanoVGBlendMode(NVGcontext* pContext, const Blend* pBlend)
{
    if (!pBlend)
    {
        nvgGlobalCompositeOperation(pContext, NVG_SOURCE_OVER);
        return;
    }

    switch (pBlend->method)
    {
        case Blend::BlendType::SrcOver:    nvgGlobalCompositeOperation(pContext, NVG_SOURCE_OVER);                break;
        case Blend::BlendType::SrcIn:      nvgGlobalCompositeOperation(pContext, NVG_SOURCE_IN);                  break;
        case Blend::BlendType::SrcOut:     nvgGlobalCompositeOperation(pContext, NVG_SOURCE_OUT);                 break;
        case Blend::BlendType::SrcAtop:    nvgGlobalCompositeOperation(pContext, NVG_ATOP);                       break;
        case Blend::BlendType::DstOver:    nvgGlobalCompositeOperation(pContext, NVG_DESTINATION_OVER);           break;
        case Blend::BlendType::DstIn:      nvgGlobalCompositeOperation(pContext, NVG_DESTINATION_IN);             break;
        case Blend::BlendType::DstOut:     nvgGlobalCompositeOperation(pContext, NVG_DESTINATION_OUT);            break;
        case Blend::BlendType::DstAtop:    nvgGlobalCompositeOperation(pContext, NVG_DESTINATION_ATOP);           break;
        case Blend::BlendType::Add:        nvgGlobalCompositeBlendFunc(pContext, NVG_SRC_ALPHA, NVG_DST_ALPHA);   break;
        case Blend::BlendType::XOR:        nvgGlobalCompositeOperation(pContext, NVG_XOR);                        break;
    }
}

NanoVGGraphics::NanoVGGraphics(juce::Component& comp)
    : attachedComponent(comp)
{
    attachedComponent.addComponentListener (this);
    juce::MessageManager::callAsync([this]()
    {
        initialise();
    });
}

NanoVGGraphics::~NanoVGGraphics()
{
    layer.reset(nullptr);
    clearFBOStack();
    onViewDestroyed();
}

void NanoVGGraphics::componentMovedOrResized (juce::Component& comp, bool, bool wasResized)
{
    if (wasResized)
    {
        windowWidth = comp.getWidth();
        windowHeight = comp.getHeight();

        if (auto* display {juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()})
            scale = std::max<float>((float)display->scale, 1.0f);

        if (nvg)
        {
            nvgReset(nvg);

            if (mainFrameBuffer != nullptr)
                nvgDeleteFramebuffer(nvg, mainFrameBuffer);

            nvgSetViewBounds(
                attachedComponent.getPeer()->getNativeHandle(),
                windowWidth,
                windowHeight);

            mainFrameBuffer = nvgCreateFramebuffer(
                nvg,
                windowWidth,
                windowHeight,
                0);
        }
    }
}

void NanoVGGraphics::timerCallback()
{
    render();
}

void NanoVGGraphics::onViewDestroyed()
{
    if (mainFrameBuffer)
        nvgDeleteFramebuffer(nvg, mainFrameBuffer);
    if (nvg)
        nvgDeleteContext(nvg);
}

void NanoVGGraphics::deleteFBO(NVGframebuffer * pBuffer)
{
    if (!inDraw)
        nvgDeleteFramebuffer(nvg, pBuffer);
    else
    {
        juce::ScopedLock sl (FBOLock);
        FBOStack.push(pBuffer);
    }
}

void NanoVGGraphics::clearFBOStack()
{
    juce::ScopedLock sl (FBOLock);
    while (!FBOStack.empty())
    {
        nvgDeleteFramebuffer(nvg, FBOStack.top());
        FBOStack.pop();
    }
}

APIBitmap *NanoVGGraphics::createBitmap(int width, int height, float scale_, float drawScale_)
{
    if (inDraw)
        nvgEndFrame(nvg);

    APIBitmap* pAPIBitmap = new APIBitmap(*this, width, height, scale_, drawScale_);

    if (inDraw)
    {
        nvgBindFramebuffer(nvg, mainFrameBuffer); // begin main frame buffer update
        // nvgBeginFrame(nvg, windowWidth, windowHeight, getScreenScale());
        nvgBeginFrame(nvg, (float)windowWidth, (float)windowHeight, 1.0f);
    }

    return pAPIBitmap;
}

void NanoVGGraphics::startLayer(ComponentLayer* component, const Rect& r)
{
    auto pixelBackingScale = scale;
    auto alignedBounds = r.getAligned();
    const int w = static_cast<int>(std::ceil(pixelBackingScale * std::ceil(alignedBounds.W())));
    const int h = static_cast<int>(std::ceil(pixelBackingScale * std::ceil(alignedBounds.H())));

    pushLayer(new Layer(createBitmap(w, h, getScreenScale(), getDrawScale()), alignedBounds, component));
}

Layer::Ptr NanoVGGraphics::endLayer()
{
    return Layer::Ptr(popLayer());
}

void NanoVGGraphics::pushLayer(Layer* pLayer)
{
    layers.push(pLayer);
    updateLayer();
}

Layer* NanoVGGraphics::popLayer()
{
    Layer* pLayer = nullptr;

    if (!layers.empty())
    {
        pLayer = layers.top();
        layers.pop();
    }

    updateLayer();

    return pLayer;
}

void NanoVGGraphics::updateLayer()
{
    if (layers.empty())
    {
        nvgEndFrame(nvg);

        nvgBindFramebuffer(nvg, mainFrameBuffer);
        // nvgBeginFrame(nvg, windowWidth, windowHeight, getScreenScale());
        nvgBeginFrame(nvg, (float)windowWidth, (float)windowHeight, 1.0f);
    }
    else
    {
        nvgEndFrame(nvg);

        nvgBindFramebuffer(nvg, layers.top()->bitmap.get()->getFBO());
#ifdef __APPLE__
        nvgClearWithColor(nvg, nvgRGBA(0, 0, 0, 0));
#endif

        nvgBeginFrame(nvg, layers.top()->getBounds().W(), layers.top()->getBounds().H(), 1.0f);
    }
}

bool NanoVGGraphics::checkLayer(const Layer::Ptr& l)
{
    const APIBitmap* bmp = l ? l->bitmap.get() : nullptr;

    if (bmp && l->component && l->componentRect != Rect(l->component->bounds))
    {
        l->componentRect = Rect(l->component->bounds);
        l->invalidate();
    }

    return bmp
        && !layer->invalid
        && bmp->drawScale == getDrawScale()
        && bmp->scale == getScreenScale();
}

void NanoVGGraphics::drawLayer(const Layer::Ptr& l, const Blend *pBlend)
{
    drawBitmap(l->bitmap.get(), l->getBounds(), 0, 0, pBlend);
}

void NanoVGGraphics::drawBitmap(const APIBitmap* bitmap, const Rect& dest, int srcX, int srcY, const Blend* pBlend)
{
    jassert(bitmap != nullptr);

    // First generate a scaled image paint
    NVGpaint imgPaint;
    float trsansformScale = 1.0f / (bitmap->scale * bitmap->drawScale);

    nvgTransformScale(imgPaint.xform, trsansformScale, trsansformScale);

    imgPaint.xform[4] = dest.L - (float)srcX;
    imgPaint.xform[5] = dest.T - (float)srcY;
    imgPaint.extent[0] = (float)bitmap->width * bitmap->scale;
    imgPaint.extent[1] = (float)bitmap->height * bitmap->scale;
    // imgPaint.image = pAPIBitmap->GetBitmap();
    imgPaint.image = bitmap->getImageId();
    imgPaint.radius = imgPaint.feather = 0.f;
    imgPaint.innerColor = imgPaint.outerColor = nvgRGBAf(1, 1, 1, Blend::blendWeight(pBlend));

    // Now draw

    nvgBeginPath(nvg); // Clears any existing path
    nvgRect(nvg, dest.L, dest.T, dest.W(), dest.H());
    nvgFillPaint(nvg, imgPaint);
    setNanoVGBlendMode(nvg, pBlend);
    nvgFill(nvg);
    nvgGlobalCompositeOperation(nvg, NVG_SOURCE_OVER);
    nvgBeginPath(nvg); // Clears the bitmap rect from the path state
}

bool NanoVGGraphics::initialise()
{
    if (nvg) return true;

    DBG("Initialising NanoVG...");

    if (attachedComponent.getBounds().isEmpty())
    {
        // Component placement is not ready yet - postpone initialisation.
        jassertfalse;
        inDraw = false;
        return false;
    }

    attachedComponent.setVisible(true);

    if (auto* display {juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()})
        scale = std::max<float>((float)display->scale, 1.0f);

    jassert(scale > 0);
    jassert(windowWidth > 0);
    jassert(windowHeight > 0);

    void* nativeHandle = attachedComponent.getPeer()->getNativeHandle();
    jassert(nativeHandle != nullptr);

    nvg = nvgCreateContext(nativeHandle, 0, windowWidth, windowHeight);
    jassert(nvg != nullptr);

    mainFrameBuffer = nvgCreateFramebuffer(
        nvg,
        windowWidth,
        windowHeight,
        0);
    jassert(mainFrameBuffer != nullptr);
    jassert(mainFrameBuffer->image > 0);

    contextCreated(nvg);

    DBG("Initialised successfully");
    return true;
}

void NanoVGGraphics::render()
{
    if (!nvg)
    {
        bool success = initialise();
        if (!success)
            return;
    }

    if (inDraw) return;

    beginFrame();
    this->draw(*this);
    endFrame();
}

void NanoVGGraphics::beginFrame()
{
    jassert(windowWidth > 0);
    jassert(windowHeight > 0);
    jassert(mainFrameBuffer != nullptr);

    inDraw = true;
    nvgBindFramebuffer(nvg, mainFrameBuffer); // begin main frame buffer update
    nvgBeginFrame(nvg, (float)windowWidth, (float)windowHeight, 1.0f);
}

void NanoVGGraphics::endFrame()
{
    nvgEndFrame(nvg); // end main frame buffer update
    nvgBindFramebuffer(nvg, nullptr);
    nvgBeginFrame(nvg, (float)windowWidth, (float)windowHeight, 1.0f);

    NVGpaint img = nvgImagePattern(nvg, 0.0f, 0.0f, (float)windowWidth, (float)windowHeight, 0.0f, mainFrameBuffer->image, 1.0f);

    nvgSave(nvg);
    nvgResetTransform(nvg);
    // nvgTranslate(nvg, mXTranslation, mYTranslation);
    nvgBeginPath(nvg);
    nvgRect(nvg, 0.0f, 0.0f, (float)windowWidth, (float)windowHeight);
    nvgFillPaint(nvg, img);
    nvgFill(nvg);
    nvgRestore(nvg);

    nvgEndFrame(nvg);

#ifdef _WIN32
    d3dPresent();
#endif

    inDraw = false;
    clearFBOStack();
}
