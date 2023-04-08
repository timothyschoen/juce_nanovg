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
            const float width = getWidth() * scale;
            const float height = getHeight() * scale;

#if NANOVG_METAL_IMPLEMENTATION
            void* nativeHandle = getPeer()->getNativeHandle();
#else
            void* nativeHandle = nullptr;
#endif

            nvgGraphicsContext.reset (new NanoVGGraphicsContext (nativeHandle, (int)width, (int)height, scale));

            contextCreated(nvgGraphicsContext->getContext());
        }
        
        if(boundsChanged)
        {
            const juce::MessageManagerLock mmLock;
            updateRenderBounds();
            boundsChanged = false;
        }

#if NANOVG_METAL_IMPLEMENTATION
        vBlank();
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
    owner->boundsChanged = owner->boundsChanged || wasResized;
}

void NanoVGComponent::initialise()
{
    jassert(!initialised);

    juce::MessageManager::callAsync([this]() {
        if (getBounds().isEmpty())
        {
            // Component placement is not ready yet - postpone initialisation.
            currentlyPainting = false;
            return;
        }
        
        if (auto* display {juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()})
            scale = (float)display->scale;
        scale = juce::jmax (1.0f,  scale);
        initialised = true;
    });
}
                                

void NanoVGComponent::updateRenderBounds()
{
    if(nvgGraphicsContext)
    {
        nvgReset(nvgGraphicsContext->getContext());

        if (auto* display {juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()})
            scale = (float)display->scale;

        nvgGraphicsContext->resized(getWidth(), getHeight(), scale);

        int scaledHeight = getHeight() * scale;
        int scaledWidth = getWidth() * scale;

        auto* nvg = nvgGraphicsContext->getContext();
        
        if(mainFB && invalidFB) {
            nvgDeleteFramebuffer(mainFB);
            nvgDeleteFramebuffer(invalidFB);
        }
        
        // Create a new framebuffer for the main context
        mainFB = nvgCreateFramebuffer(nvg, scaledWidth, scaledHeight, 0);
 
        // Create a new framebuffer for the invalid area
        invalidFB = nvgCreateFramebuffer(nvg, scaledWidth, scaledHeight, 0);
        
#if NANOVG_METAL_IMPLEMENTATION
        mnvgSetViewBounds(getPeer()->getNativeHandle(), juce::roundToInt(scale * getWidth()), juce::roundToInt(scale * getHeight()));
#endif
    }
}

void NanoVGComponent::render()
{
    if (!initialised)
    {
        initialise();
    }
    
    if(!nvgGraphicsContext) return;

    auto invalidBounds = invalidArea.getBounds();

    int x = invalidBounds.getX() * scale;
    int y = invalidBounds.getY() * scale;
    int w = invalidBounds.getWidth() * scale;
    int h = invalidBounds.getHeight() * scale;
    
    int scaledHeight = getHeight() * scale;
    int scaledWidth = getWidth() * scale;

    auto* nvg = nvgGraphicsContext->getContext();


    if(!mainFB || !invalidFB) {
        // Create a new framebuffer for the main context
        mainFB = nvgCreateFramebuffer(nvg, scaledWidth, scaledHeight, 0);

        // Create a new framebuffer for the invalid area
        invalidFB = nvgCreateFramebuffer(nvg, scaledWidth, scaledHeight, 0);
    }
    
    juce::Graphics fbGraphics(*nvgGraphicsContext.get());
    const juce::MessageManagerLock mmLock;

    if(mmLock.lockWasGained() && !invalidBounds.isEmpty())
    {
        // Save the current state of the context
        nvgSave(nvg);
        
        // Bind the new framebuffer to the context
        nvgBindFramebuffer(invalidFB);
        
        // Render to the new framebuffer
        nvgBeginFrame(nvg, scaledWidth, scaledHeight, scale);
        
        //nvgClearWithColor(nvg, nvgRGBA(0.0f, 0.0f, 0.0f, 0.0f));
        
        fbGraphics.getInternalContext().clipToRectangle(invalidBounds);
        paintEntireComponent(fbGraphics, true);
        
#if JUCE_ENABLE_REPAINT_DEBUGGING
        static juce::Random rng;
        fbGraphics.fillAll (juce::Colour ((juce::uint8) rng.nextInt (255), (juce::uint8) rng.nextInt (255), (juce::uint8) rng.nextInt (255), (juce::uint8) 0x50));
#endif
        
        nvgEndFrame(nvg);
        
        // Draw invalid area to mainFB
        nvgBindFramebuffer(mainFB);
        nvgBeginFrame(nvg, scaledWidth, scaledHeight, 1.0f);
        nvgBeginPath(nvg);
        nvgRect(nvg, x, y, w, h);
        nvgFillPaint(nvg, nvgImagePattern(nvg, 0, 0, scaledWidth, scaledHeight, 0, invalidFB->image, 1));
        nvgFill(nvg);
        nvgEndFrame(nvg);
    }
    
    needsRepaint = true;
    invalidArea.clear();
}

void NanoVGComponent::vBlank()
{
    if(!needsRepaint) return;
    
    int scaledHeight = getHeight() * scale;
    int scaledWidth = getWidth() * scale;
    
    auto* nvg = nvgGraphicsContext->getContext();
    
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

    #if NANOVG_GL_IMPLEMENTATION
        if (!openGLContext.isActive())
            openGLContext.makeActive();
    #endif
    
    needsRepaint = false;
}

void NanoVGComponent::shutdown()
{
    //nvgDeleteContext(nvg);
}

//==============================================================================

