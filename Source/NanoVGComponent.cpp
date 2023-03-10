//
//  Copyright (C) 2022 Arthur Benilov <arthur.benilov@gmail.com> and Timothy Schoen <timschoen123@gmail.com>
//

#include "NanoVGComponent.h"

#if NANOVG_METAL_IMPLEMENTATION
#include <nanovg_mtl.h>
#endif


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
            const float width = getWidth();
            const float height = getHeight();

            #if NANOVG_METAL_IMPLEMENTATION
            void* nativeHandle = getPeer()->getNativeHandle();
            #else
            void* nativeHandle = nullptr;
            #endif

            nvgGraphicsContext.reset (new NanoVGGraphicsContext (nativeHandle, (int)width, (int)height));
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
    }
}

void NanoVGComponent::resized ()
{
    if (!initialised)
        return;

    if (nvgGraphicsContext != nullptr) {
        nvgReset(nvgGraphicsContext->getContext());
        nvgGraphicsContext->resized(getWidth(), getHeight());
    }

    #if NANOVG_METAL_IMPLEMENTATION
    mnvgSetViewBounds(getPeer()->getNativeHandle(), getWidth(), getHeight());
    #endif
}

void NanoVGComponent::initialise()
{
    if (initialised)
        return;

    juce::MessageManager::callAsync([this]()
    {
        if (getBounds().isEmpty())
        {
            // Component placement is not ready yet - postpone initialisation.
            currentlyPainting = false;
            return;
        }

        setVisible(true);

        initialised = true;
    });
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
