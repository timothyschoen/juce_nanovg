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

APIBitmap *NanoVGGraphics::createBitmap(int width, int height, float scale, double drawScale)
{
    if (inDraw)
        nvgEndFrame(nvg);

    APIBitmap* pAPIBitmap = new APIBitmap(nvg, width, height, scale, drawScale);

    if (inDraw)
    {
        mnvgBindFramebuffer(mainFrameBuffer); // begin main frame buffer update
        nvgBeginFrame(nvg, windowWidth, windowHeight, getScreenScale());
    }

    return pAPIBitmap;
}

void NanoVGGraphics::startLayer(ComponentLayer* component, const Rect& r, bool cacheable)
{
    auto pixelBackingScale = scale;
    auto alignedBounds = r.getAligned();
    const int w = static_cast<int>(std::ceil(pixelBackingScale * std::ceil(alignedBounds.W())));
    const int h = static_cast<int>(std::ceil(pixelBackingScale * std::ceil(alignedBounds.H())));

    pushLayer(new Layer(createBitmap(w, h, getScreenScale(), getDrawScale()), alignedBounds, component));
}

void NanoVGGraphics::resumeLayer(Layer::Ptr &layer)
{
    Layer::Ptr ownedLayer;
    
    ownedLayer.swap(layer);

    if (Layer* ownerlessLayer = ownedLayer.release())
        pushLayer(ownerlessLayer);
}

Layer::Ptr NanoVGGraphics::endLayer()
{
    return Layer::Ptr(popLayer());
}

void NanoVGGraphics::pushLayer(Layer* pLayer)
{
    layers.push(pLayer);
    updateLayer();
    pathTransformReset();
    pathClipRegion(pLayer->getBounds());
    pathClear();
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
    pathTransformReset();
    pathClipRegion();
    pathClear();

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
        nvgBeginFrame(nvg, windowWidth, windowHeight, getScreenScale());
    }
    else
    {
        nvgEndFrame(nvg);
        // #ifdef IGRAPHICS_GL
        // const double scale = GetBackingPixelScale();
        // glViewport(0, 0, mLayers.top()->Bounds().W() * scale, mLayers.top()->Bounds().H() * scale);
        // #endif
        mnvgBindFramebuffer(layers.top()->bitmap.get()->getFBO());
        nvgBeginFrame(nvg, layers.top()->getBounds().W() * getDrawScale(), layers.top()->getBounds().H() * getDrawScale(), getScreenScale());
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
    pathTransformSave();
    pathTransformReset();
    drawBitmap(layer->bitmap.get(), layer->getBounds(), 0, 0, pBlend);
    pathTransformRestore();
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

void NanoVGGraphics::setClipRegion(const Rect &r)
{
    nvgScissor(nvg, r.L, r.T, r.W(), r.H());
}

void NanoVGGraphics::pathClipRegion(const Rect r)
{
    Rect drawArea = layers.empty() ? clipRect : layers.top()->getBounds();
    Rect clip = r.isEmpty() ? drawArea : r.Intersect(drawArea);
    pathTransformSetMatrix(Matrix());
    setClipRegion(clip);
    pathTransformSetMatrix(transform);
}

void NanoVGGraphics::pathTransformSave()
{
    transformStates.push(transform);
}

void NanoVGGraphics::pathTransformReset(bool clearStates)
{
    if (clearStates)
  {
    std::stack<Matrix> newStack;
    transformStates.swap(newStack);
  }
  
  transform = Matrix();
  pathTransformSetMatrix(transform);
}

void NanoVGGraphics::pathTransformRestore()
{
    if (!transformStates.empty())
    {
        transform = transformStates.top();
        transformStates.pop();
        pathTransformSetMatrix(transform);
    }
}

void NanoVGGraphics::pathClear()
{
    nvgBeginPath(nvg);
}

void NanoVGGraphics::pathTransformSetMatrix(const Matrix& m)
{
    double xTranslate = 0.0;
    double yTranslate = 0.0;

    if (!layers.empty())
    {
        Rect bounds = layers.top()->getBounds();

        xTranslate = -bounds.L;
        yTranslate = -bounds.T;
    }

    nvgResetTransform(nvg);
    nvgScale(nvg, getDrawScale(), getDrawScale());
    nvgTranslate(nvg, xTranslate, yTranslate);
    nvgTransform(nvg, m.mXX, m.mYX, m.mXY, m.mYY, m.mTX, m.mTY);
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

    mnvgBindFramebuffer(mainFrameBuffer);
    mnvgClearWithColor(nvg, nvgRGBA(0, 0, 0, 0));

    nvgBeginFrame(nvg, windowWidth, windowHeight, 1.0f);
    nvgEndFrame(nvg);

    contextCreated(nvg);

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

    inDraw = false;
}

void NanoVGGraphics::beginFrame()
{
    jassert(windowWidth > 0);
    jassert(windowHeight > 0);
    jassert(mainFrameBuffer != nullptr);

    inDraw = true;
    mnvgBindFramebuffer(mainFrameBuffer); // begin main frame buffer update
    mnvgClearWithColor(nvg, nvgRGBA(0, 0, 0, 0));
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
}
