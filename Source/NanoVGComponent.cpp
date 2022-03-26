//
//  Copyright (C) 2022 Arthur Benilov <arthur.benilov@gmail.com> and Timothy Schoen <timschoen123@gmail.com>
//

#include "NanoVGComponent.h"

#if NANOVG_METAL_IMPLEMENTATION
#include <nanovg_mtl.h>
#endif


void NanoVGComponent::startPeriodicRepaint (int fps)
{
    if (fps > 0)
        startTimerHz (fps);
    else
        stopTimer();
}

void NanoVGComponent::stopPeriodicRepaint()
{
    stopTimer();
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

#if NANOVG_METAL_IMPLEMENTATION
    render();
#else
    openGLContext.triggerRepaint();
#endif

    currentlyPainting = false;
}

void NanoVGComponent::timerCallback()
{
    repaint();
}

NanoVGComponent::NanoVGComponent()
{
    setOpaque (true);
    addComponentListener (this);
    
#if NANOVG_METAL_IMPLEMENTATION
    setCachedComponentImage (new RenderCache (*this));

#elif NANOVG_GL_IMPLEMENTATION
    setCachedComponentImage (new RenderCache (*this));
    openGLContext.setOpenGLVersionRequired(juce::OpenGLContext::openGL3_2);
    openGLContext.setComponentPaintingEnabled(false);
    openGLContext.setMultisamplingEnabled(false);
    openGLContext.setContinuousRepainting (true);
#endif
}

NanoVGComponent::~NanoVGComponent()
{
    removeComponentListener(this);

#if NANOVG_GL_IMPLEMENTATION
    openGLContext.detach();
#endif

    if (nvg != nullptr)
    {
        nvgGraphicsContext->removeCachedImages();
        //nvgDeleteContext(nvg);
    }
}

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
    return invalidateAll();
}

void NanoVGComponent::RenderCache::releaseResources()
{
}

void NanoVGComponent::RenderCache::handleAsyncUpdate()
{
    component.paintComponent();
}


void NanoVGComponent::componentMovedOrResized (juce::Component& component, bool wasMoved, bool wasResized)
{
    if (!initialised)
        return;
    
    if (wasResized && nvg) {
        nvgReset(nvg);
        
        nvgGraphicsContext->resized(getWidth(), getHeight());
    }
    
    if (auto* display {juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()})
        scale = display->scale;

    const auto width = uint32_t (scale * getWidth());
    const auto height = uint32_t (scale * getHeight());
    #if NANOVG_METAL_IMPLEMENTATION
      mnvgSetViewBounds(getPeer()->getNativeHandle(), width, height);
    #endif
}



void NanoVGComponent::initialise()
{
    juce::MessageManager::callAsync([this](){
        
        if (initialised)
            return;
        
        if (getBounds().isEmpty())
        {
            // Component placement is not ready yet - postpone initialisation.
            currentlyPainting = false;
            return;
        }

        setVisible(true);
        
        if (auto* display {juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()})
            scale = display->scale;
        scale = juce::jmax (1.0f,  scale);


        initialised = true;
    });

}

void NanoVGComponent::render()
{
    if(!initialised) return;
    
    if (!nvg)
    {
        const float width {getWidth() * scale};
        const float height {getHeight() * scale};
        
        #if NANOVG_METAL_IMPLEMENTATION
        void* nativeHandle = getPeer()->getNativeHandle();
        #else
        void* nativeHandle = nullptr;
        #endif
        
        nvgGraphicsContext.reset (new NanoVGGraphicsContext (nativeHandle, (int)width, (int)height));
        nvg = nvgGraphicsContext->getContext();
        
        //mainFrameBuffer = nvgCreateFramebuffer(nvg, width, height, 0);
    }
#if NANOVG_GL_IMPLEMENTATION
    glViewport(0, 0, getWidth() * scale, getHeight() * scale);
    glClearColor(0,0,0,0);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
#endif
    
    nvgScissor(nvg, 0, 0, getWidth(), getHeight());
    nvgBeginFrame (nvg, getWidth(), getHeight(), scale);
    
    juce::MessageManager::Lock mmLock;
    //if(mmLock.tryEnter()) {
        juce::Graphics g (*nvgGraphicsContext.get());
        paintEntireComponent (g, true);
        //mmLock.exit();

    //}/Users/timschoen/Projecten/juce_nanovg/Source/NanoVGGraphics.cpp
    /*
    int x = 10, y = 10, w = 200, h = 200;
    float pos = 1.0f;
    
    float cy = y+(int)(h*0.5f);
        float kr = (int)(h*0.25f);
    
    NVGpaint knob = nvgLinearGradient(nvg, x,cy-kr,x,cy+kr, nvgRGBA(255,255,255,16), nvgRGBA(0,0,0,16));
    nvgBeginPath(nvg);
    nvgCircle(nvg, x+(int)(pos*w),cy, kr-1);
    nvgFillColor(nvg, nvgRGBA(40,43,48,255));
    nvgFill(nvg);
    nvgFillPaint(nvg, knob);
    nvgFill(nvg);

    nvgBeginPath(nvg);
    nvgCircle(nvg, x+(int)(pos*w),cy, kr-0.5f);
    nvgStrokeColor(nvg, nvgRGBA(0,0,0,92));
    nvgStroke(nvg); */
    
    nvgEndFrame (nvg);
}

void NanoVGComponent::shutdown()
{
    //nvgDeleteContext(nvg);
}

