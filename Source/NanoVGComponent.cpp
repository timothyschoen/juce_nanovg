//
//  Copyright (C) 2022 Arthur Benilov <arthur.benilov@gmail.com> and Timothy Schoen <timschoen123@gmail.com>
//
#include "NanoVGComponent.h"


NanoVGComponent::NanoVGComponent()
{
    setOpaque (true);
    addComponentListener (this);

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
    removeComponentListener(this);

#if NANOVG_GL_IMPLEMENTATION
    openGLContext.detach();
#endif

    if (nvgGraphicsContext != nullptr)
    {
        nvgGraphicsContext->removeCachedImages();
        //nvgDeleteContext(nvg);
    }
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

            //mainFrameBuffer = nvgCreateFramebuffer(nvg, width, height, 0);
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

void NanoVGComponent::timerCallback()
{
    repaint();
}

void NanoVGComponent::componentMovedOrResized (juce::Component&, bool, bool wasResized)
{
    if (!initialised)
        return;

    if (wasResized && nvgGraphicsContext)
    {
        nvgReset(nvgGraphicsContext->getContext());

        if (auto* display {juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()})
            scale = (float)display->scale;

        nvgGraphicsContext->resized(getWidth(), getHeight(), scale);

#if NANOVG_METAL_IMPLEMENTATION
        mnvgSetViewBounds(getPeer()->getNativeHandle(), juce::roundToInt(scale * getWidth()), juce::roundToInt(scale * getHeight()));
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

#if NANOVG_GL_IMPLEMENTATION
    glViewport(0, 0, getWidth() * scale, getHeight() * scale);
    glClearColor(0,0,0,0);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
#endif

    //nvgScissor(nvg, 0, 0, getWidth(), getHeight());
    nvgBeginFrame (nvgGraphicsContext->getContext(), getWidth(), getHeight(), scale);

    juce::MessageManager::Lock mmLock;
    juce::Graphics g (*nvgGraphicsContext.get());
    paintEntireComponent (g, true);

#if NANOVG_GL_IMPLEMENTATION
    if (!openGLContext.isActive())
        openGLContext.makeActive();
#endif

    nvgEndFrame (nvgGraphicsContext->getContext());
    //openGLContext.swapBuffers();
}

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
    triggerAsyncUpdate();
    return true;
}

bool NanoVGComponent::RenderCache::invalidate (const juce::Rectangle<int>&)
{
    // TODO: more selective invalidation
    return invalidateAll();
}

void NanoVGComponent::RenderCache::releaseResources()
{
}

void NanoVGComponent::RenderCache::handleAsyncUpdate()
{
    component.paintComponent();
}
