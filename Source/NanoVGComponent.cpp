//
//  Copyright (C) 2022 Arthur Benilov <arthur.benilov@gmail.com> and Timothy Schoen <timschoen123@gmail.com>
//
#include "NanoVGComponent.h"

NanoVGComponent::NanoVGComponent()
{
    setOpaque (true);

#if NANOVG_METAL_IMPLEMENTATION
    setCachedComponentImage (new RenderCache (*this));
#elif NANOVG_GL_IMPLEMENTATION
    setCachedComponentImage (new RenderCache (*this));

#if NANOVG_GL3_IMPLEMENTATION || NANOVG_GLES3_IMPLEMENTATION
    openGLContext.setOpenGLVersionRequired(juce::OpenGLContext::openGL3_2);
#elif NANOVG_GL2_IMPLEMENTATION || NANOVG_GLES2_IMPLEMENTATION
    openGLContext.setOpenGLVersionRequired(juce::OpenGLContext::defaultGLVersion);
#endif
    openGLContext.setComponentPaintingEnabled(false);
    openGLContext.setMultisamplingEnabled(true);
    openGLContext.setContinuousRepainting (false);
#endif
}

NanoVGComponent::~NanoVGComponent()
{

#if NANOVG_GL_IMPLEMENTATION
    openGLContext.detach();
#endif

    if (nvgGraphicsContext != nullptr)
    {
        nvgGraphicsContext->removeCachedImages();
        //nvgDeleteContext(nvg);
    }
}

void NanoVGComponent::addFont(const juce::String& name, const char* data, size_t size)
{
    nvgGraphicsContext->loadFont(name, data, static_cast<int>(size));
}

void NanoVGComponent::paintComponent()
{
    if (currentlyPainting)
        return;

    currentlyPainting = true;

    if (!initialised)
    {
        initialise();
    }
    else
    {
        if (nvgGraphicsContext == nullptr)
        {
            const float width {getWidth() * scale};
            const float height {getHeight() * scale};

#if NANOVG_METAL_IMPLEMENTATION
            void* nativeHandle = getPeer()->getNativeHandle();
#else
            void* nativeHandle = nullptr;
#endif

            nvgGraphicsContext.reset (new NanoVGGraphicsContext (nativeHandle, (int)width, (int)height, scale));

            contextCreated(nvgGraphicsContext->getContext());
        }

#if NANOVG_METAL_IMPLEMENTATION
        render();
#else
        openGLContext.triggerRepaint();
#endif
    }

    currentlyPainting = false;
}

void NanoVGComponent::ComponentUpdater::timerCallback()
{
    owner->repaint();
}

void NanoVGComponent::ComponentUpdater::componentMovedOrResized (juce::Component&, bool, bool wasResized)
{
    if (!owner->initialised)
        return;

    if (wasResized && owner->nvgGraphicsContext)
    {
        nvgReset(owner->nvgGraphicsContext->getContext());

        if (auto* display {juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()})
            owner->scale = (float)display->scale;

        owner->nvgGraphicsContext->resized(owner->getWidth(), owner->getHeight(), owner->scale);

#if NANOVG_METAL_IMPLEMENTATION
        mnvgSetViewBounds(owner->getPeer()->getNativeHandle(), juce::roundToInt(owner->scale * owner->getWidth()), juce::roundToInt(owner->scale * owner->getHeight()));
#endif
    }
}

void NanoVGComponent::initialise()
{
    jassert(!initialised);

    juce::MessageManager::callAsync([this]()
    {
        if (getBounds().isEmpty())
        {
            // Component placement is not ready yet - postpone initialisation.
            currentlyPainting = false;
            return;
        }

        setVisible(true);

        if (auto* display {juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()})
            scale = (float)display->scale;
        scale = juce::jmax (1.0f,  scale);
        initialised = true;
    });
}



void NanoVGComponent::render()
{
    jassert(initialised);

    auto invalidBounds = invalidArea.getBounds();
    int x = invalidBounds.getX() * scale;
    int y = invalidBounds.getY() * scale;
    int w = invalidBounds.getWidth() * scale;
    int h = invalidBounds.getHeight() * scale;
    
    int scaledHeight = getHeight() * scale;
    int scaledWidth = getWidth() * scale;

    auto* nvg = nvgGraphicsContext->getContext();

    // Save the current state of the context
    nvgSave(nvg);

    if(!mainFB) {
        // Create a new framebuffer for the main context
        mainFB = nvgCreateFramebuffer(nvg, scaledWidth, scaledHeight, 0);
    }

    // Bind the new framebuffer to the context
    nvgBindFramebuffer(mainFB);

    juce::MessageManager::Lock mmLock;
    juce::Graphics fbGraphics(*nvgGraphicsContext.get());

    // Create a new framebuffer for the invalid area
    NVGframebuffer* invalidFB = nvgCreateFramebuffer(nvg, scaledWidth, scaledHeight, 0);

    // Bind the new framebuffer to the context
    nvgBindFramebuffer(invalidFB);

    // Render to the new framebuffer
    nvgBeginFrame(nvg, scaledWidth, scaledHeight, scale);

    fbGraphics.reduceClipRegion(invalidBounds);
    paintEntireComponent(fbGraphics, true);
    
    //static juce::Random rng;
    //fbGraphics.fillAll (juce::Colour ((juce::uint8) rng.nextInt (255), (juce::uint8) rng.nextInt (255), (juce::uint8) rng.nextInt (255), (juce::uint8) 0x50));

    nvgEndFrame(nvg);
    
    // Draw invalid area to mainFB
    nvgBindFramebuffer(mainFB);
    nvgBeginFrame(nvg, scaledWidth, scaledHeight, 1.0f);
    nvgBeginPath(nvg);
    nvgRect(nvg, x, y, w, h);
    nvgFillPaint(nvg, nvgImagePattern(nvg, 0, 0, scaledWidth, scaledHeight, 0, invalidFB->image, 1));
    nvgFill(nvg);
    nvgEndFrame(nvg);

    // Bind the default framebuffer to the context (to draw to the screen)
    nvgBindFramebuffer(0);

    // Restore the previous state of the context
    nvgRestore(nvg);

    // Draw the main framebuffer onto the screen
    nvgBeginFrame(nvg, scaledWidth, scaledHeight, 1.0f);
    nvgBeginPath(nvg);
    nvgClearWithColor(nvg, nvgRGBAf(0.0f, 0.0f, 0.0f, 0.0f));
    nvgRect(nvg, 0, 0, scaledWidth, scaledHeight);
    nvgFillPaint(nvg, nvgImagePattern(nvg, 0, 0, scaledWidth, scaledHeight, 0, mainFB->image, 1));
    nvgFill(nvg);
    nvgEndFrame(nvg);

    // Delete the framebuffers
    //nvgDeleteFramebuffer(mainFB);
    nvgDeleteFramebuffer(invalidFB);

    invalidArea.clear();
}

//void NanoVGComponent::render()
//{
//    jassert(initialised);
//
//
//#if NANOVG_GL_IMPLEMENTATION
//    glViewport(0, 0, getWidth() * scale, getHeight() * scale);
//    glClearColor(0,0,0,0);
//    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
//#endif
//
//    auto compBounds = getLocalBounds();
//    auto invalidBounds = invalidArea.getBounds();
//
//    auto* nvg = nvgGraphicsContext->getContext();
//
//    // Render the component to a framebuffer
//    juce::MessageManager::Lock mmLock;
//    juce::Graphics g (*nvgGraphicsContext.get());
//
//    auto* fb = nvgCreateFramebuffer(nvg, getWidth(), getHeight(), 0);
//
//
//    //nvgBindFramebuffer(fb);
//    nvgBeginFrame(nvg, getWidth(), getHeight(), scale);
//    //nvgClearWithColor(nvg, nvgRGBAf(0.0f, 0.0f, 0.0f, 0.0f)); // Replace with your desired clear color
//
//    /*
//    juce::Graphics fbGraphics (*nvgGraphicsContext.get());
//    // TODO: apply scissor here to clip it
//    // This is still going wrong at the moment
//    nvgScissor(nvg, invalidBounds.getX(), invalidBounds.getY(), invalidBounds.getWidth(), invalidBounds.getHeight());
//    paintEntireComponent(fbGraphics, true);
//    nvgResetScissor(nvg); */
//
//    static juce::Random rng;
//
//    //fbGraphics.fillAll (juce::Colour ((juce::uint8) rng.nextInt (255), (juce::uint8) rng.nextInt (255), (juce::uint8) rng.nextInt (255), (juce::uint8) 0x50));
//
//    // Render the invalidated area to the main context
//    //nvgBindFramebuffer(NULL);
//
//    nvgSave(nvg);
//    //nvgResetTransform(nvg);
//
//    nvgGlobalCompositeOperation(nvg, NVG_SOURCE_OVER);
//
//    //nvgTranslate(nvg, 0, 0);
//    //nvgBeginPath(nvg);
//    //nvgRect(nvg, 0, 0, getWidth(), getHeight());
//
//    nvgBeginPath (nvg);
//    nvgRect (nvg, invalidBounds.getX(), invalidBounds.getY(), invalidBounds.getWidth(), invalidBounds.getHeight());
//    //nvgFillPaint (nvg, nvgImagePattern(nvg, invalidBounds.getX(), invalidBounds.getY(), invalidBounds.getWidth(), invalidBounds.getHeight(), 0, fb->image, 1));
//    nvgFillColor(nvg, nvgRGBA(255, 0, 0, 255));
//    nvgFill(nvg);
//
//
//    nvgRestore(nvg);
//    nvgEndFrame (nvg);
//    invalidArea.clear();
//
//    nvgDeleteFramebuffer(fb);
//
//    /*
//    //nvgScissor(nvg, 0, 0, getWidth(), getHeight());
//    nvgBeginFrame (nvgGraphicsContext->getContext(), getWidth(), getHeight(), scale);
//
//    juce::MessageManager::Lock mmLock;
//    juce::Graphics g (*nvgGraphicsContext.get());
//
//
//    //scale = g.getInternalContext().getPhysicalPixelScaleFactor();
//    auto compBounds = getLocalBounds();
//    auto invalidBounds = invalidArea.getBounds();
//
//    if (compBounds.intersects(invalidBounds))
//    {
//        auto& lg = g.getInternalContext();
//
//        //lg.addTransform (juce::AffineTransform::scale (scale));
//
//        //validArea.subtract(compBounds);
//
//        lg.clipToRectangle(invalidBounds);
//
//        //for (auto& i : validArea)
//        //    lg.excludeClipRectangle(i);
//
//        if (! isOpaque())
//        {
//            lg.setFill (juce::Colours::transparentBlack);
//            lg.fillRect (compBounds, true);
//            lg.setFill (juce::Colours::black);
//        }
//
//        paintEntireComponent (g, true);
//    }
//
//
//
//#if NANOVG_GL_IMPLEMENTATION
//    if (!openGLContext.isActive())
//        openGLContext.makeActive();
//#endif
//
//    nvgEndFrame (nvgGraphicsContext->getContext());
//    //openGLContext.swapBuffers(); */
//}

void NanoVGComponent::shutdown()
{
    //nvgDeleteContext(nvg);
}

//==============================================================================

NanoVGComponent::RenderCache::RenderCache (NanoVGComponent& comp)
    : component {comp}
{
}

NanoVGComponent::RenderCache::~RenderCache()
{
    cancelPendingUpdate();
}

void NanoVGComponent::RenderCache::paint (juce::Graphics&)
{
    component.paintComponent();
}

bool NanoVGComponent::RenderCache::invalidateAll()
{
    component.invalidArea = component.getLocalBounds();
    triggerAsyncUpdate();
    return true;
}

bool NanoVGComponent::RenderCache::invalidate (const juce::Rectangle<int>& rect)
{
    component.invalidArea.add(rect);
    triggerAsyncUpdate();
    return true;
}

void NanoVGComponent::RenderCache::releaseResources()
{
}

void NanoVGComponent::RenderCache::handleAsyncUpdate()
{
    component.paintComponent();
}
