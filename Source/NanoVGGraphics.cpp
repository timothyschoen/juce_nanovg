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
            scale = std::max((float)display->scale, 1.0f);

        if (nvg)
        {
            nvgReset(nvg);

            if (mainFrameBuffer != nullptr)
                mnvgDeleteFramebuffer(mainFrameBuffer);

            mnvgSetViewBounds(
                attachedComponent.getPeer()->getNativeHandle(),
                windowWidth,
                windowHeight);

            mainFrameBuffer = mnvgCreateFramebuffer(
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
        mnvgDeleteFramebuffer(mainFrameBuffer);
    if (nvg)
        nvgDeleteMTL(nvg);
}

void NanoVGGraphics::deleteFBO(MNVGframebuffer * pBuffer)
{
    if (!inDraw)
        mnvgDeleteFramebuffer(pBuffer);
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
        mnvgDeleteFramebuffer(FBOStack.top());
        FBOStack.pop();
    }
}

APIBitmap *NanoVGGraphics::createBitmap(int width, int height, float scale_, double drawScale_)
{
    if (inDraw)
        nvgEndFrame(nvg);

    APIBitmap* pAPIBitmap = new APIBitmap(*this, width, height, scale_, drawScale_);

    if (inDraw)
    {
        mnvgBindFramebuffer(mainFrameBuffer); // begin main frame buffer update
        // nvgBeginFrame(nvg, windowWidth, windowHeight, getScreenScale());
        nvgBeginFrame(nvg, windowWidth, windowHeight, 1.0f);
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
        // #ifdef IGRAPHICS_GL
        // glViewport(0, 0, WindowWidth() * GetScreenScale(), WindowHeight() * GetScreenScale());
        // #endif
        mnvgBindFramebuffer(mainFrameBuffer);
        // nvgBeginFrame(nvg, windowWidth, windowHeight, getScreenScale());
        nvgBeginFrame(nvg, windowWidth, windowHeight, 1.0f);
    }
    else
    {
        nvgEndFrame(nvg);
        // #ifdef IGRAPHICS_GL
        // const double scale = GetBackingPixelScale();
        // glViewport(0, 0, mLayers.top()->Bounds().W() * scale, mLayers.top()->Bounds().H() * scale);
        // #endif
        mnvgBindFramebuffer(layers.top()->bitmap.get()->getFBO());
        mnvgClearWithColor(nvg, nvgRGBA(0, 0, 0, 0));
        nvgBeginFrame(nvg, layers.top()->getBounds().W(), layers.top()->getBounds().H(), 1.0f);
    }
}

bool NanoVGGraphics::checkLayer(const Layer::Ptr &layer)
{
    const APIBitmap* bmp = layer ? layer->bitmap.get() : nullptr;

    if (bmp && layer->component && layer->componentRect != Rect(layer->component->bounds))
    {
        layer->componentRect = Rect(layer->component->bounds);
        layer->invalidate();
    }

    return bmp
        && !layer->invalid
        && bmp->drawScale == getDrawScale()
        && bmp->scale == getScreenScale();
}

void NanoVGGraphics::drawLayer(const Layer::Ptr &layer, const Blend *pBlend)
{
    drawBitmap(layer->bitmap.get(), layer->getBounds(), 0, 0, pBlend);
}

void NanoVGGraphics::drawBitmap(const APIBitmap* bitmap, const Rect& dest, int srcX, int srcY, const Blend* pBlend)
{
    jassert(bitmap != nullptr);

    // First generate a scaled image paint
    NVGpaint imgPaint;
    double trsansformScale = 1.0 / (bitmap->scale * bitmap->drawScale);

    nvgTransformScale(imgPaint.xform, trsansformScale, trsansformScale);

    imgPaint.xform[4] = dest.L - srcX;
    imgPaint.xform[5] = dest.T - srcY;
    imgPaint.extent[0] = bitmap->width * bitmap->scale;
    imgPaint.extent[1] = bitmap->height * bitmap->scale;
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

void NanoVGGraphics::initialise()
{
    if (nvg) return;
    DBG("Initialising NanoVG...");

    if (attachedComponent.getBounds().isEmpty())
    {
        // Component placement is not ready yet - postpone initialisation.
        jassertfalse;
        inDraw = false;
        return;
    }

    attachedComponent.setVisible(true);

    if (auto* display {juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()})
        scale = std::max((float)display->scale, 1.0f);

    jassert(scale > 0);
    jassert(windowWidth > 0);
    jassert(windowHeight > 0);

    void* nativeHandle = attachedComponent.getPeer()->getNativeHandle();
    jassert(nativeHandle != nullptr);
    // onViewInitialised(nativeHandle, getWidth(), getHeight());
    nvg = mnvgCreateContext(nativeHandle, 0, windowWidth, windowHeight);
    mainFrameBuffer = mnvgCreateFramebuffer(
        nvg,
        windowWidth,
        windowHeight,
        0);

    contextCreated(nvg);

    render();

    DBG("Initialised successfully");
}

void NanoVGGraphics::render()
{
    if (!nvg)
    {
        jassertfalse;
        initialise();
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
    mnvgBindFramebuffer(mainFrameBuffer); // begin main frame buffer update
    nvgBeginFrame(nvg, windowWidth, windowHeight, 1.0f);
}

void NanoVGGraphics::endFrame()
{
    nvgEndFrame(nvg); // end main frame buffer update
    mnvgBindFramebuffer(nullptr);
    nvgBeginFrame(nvg, windowWidth, windowHeight, 1.0f);

    NVGpaint img = nvgImagePattern(nvg, 0, 0, windowWidth, windowHeight, 0, mainFrameBuffer->image, 1.0f);

    nvgSave(nvg);
    nvgResetTransform(nvg);
    // nvgTranslate(nvg, mXTranslation, mYTranslation);
    nvgBeginPath(nvg);
    nvgRect(nvg, 0, 0, windowWidth, windowHeight);
    nvgFillPaint(nvg, img);
    nvgFill(nvg);
    nvgRestore(nvg);

    nvgEndFrame(nvg);

    inDraw = false;
    clearFBOStack();
}
