//
//  Copyright (C) 2022 Arthur Benilov <arthur.benilov@gmail.com> and Timothy Schoen <timschoen123@gmail.com>
//


#include "NanoVGComponent.h"

#if NANOVG_METAL
#include <nanovg_mtl.h>

#elif JUCE_WINDOWS
#include <Windows.h>

namespace juce {
    extern ComponentPeer* createNonRepaintingEmbeddedWindowsPeer (Component&, void* parent);
}
#endif

bool NanoVGComponent::isInitialised() const
{
    return initialised;
}

void NanoVGComponent::enableRenderStats()
{
    // Enabling the rendering statistics can be done only before the component
    // is initialised. Enabling it afterwards will have no effect.
    jassert(!isInitialised());
    showRenderStats = true;
}

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

void NanoVGComponent::initialise()
{
#if NANOVG_METAL
    if (getBounds().isEmpty())
    {
        // Component placement is not ready yet - postpone initialisation.
        currentlyPainting = false;
        return;
    }

    setBackgroundColour (backgroundColour);

    if (auto* display {juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()})
        scale = display->scale;

    if (attachedComponent == nullptr || initialised)
        return;

    scale = juce::jmax (1.0f, scale);

    overlay.setVisible (true);

#elif JUCE_WINDOWS
    nativeWindow.reset (createNonRepaintingEmbeddedWindowsPeer (overlay, overlay.getTopLevelComponent()->getWindowHandle()));
    nativeWindow->setVisible (true);
#endif
    
#if JUCE_MAC
    auto* peer =  overlay.getPeer();
#elif JUCE_WINDOWS
    auto* peer = nativeWindow.get();
#else
    auto* peer = nullptr;
#endif
    
#if NANOVG_METAL
    embeddedView.setView (peer->getNativeHandle());
#endif

    initialised = true;

    trackOverlay (false, true);

    //overlay.getTopLevelComponent()->repaint();
}

void NanoVGComponent::render()
{
    if (!nvg)
    {
        const float width {getWidth() * scale};
        const float height {getHeight() * scale};
        
        #if NANOVG_METAL
            void* nativeHandle = embeddedView.getView();
        #elif JUCE_WINDOWS
            void* nativeHandle = nativeWindow->getNativeHandle();
        #else
        void* nativeHandle = nullptr;
        #endif
        
        nvgGraphicsContext.reset (new NanoVGGraphicsContext (nativeHandle, (int)width, (int)height));
        nvg = nvgGraphicsContext->getContext();
        
        //mainFrameBuffer = nvgCreateFramebuffer(nvg, width, height, 0);
    }
    
    glViewport(0, 0, getWidth() * scale, getHeight() * scale);
    
    nvgBeginFrame (nvg, getWidth(), getHeight(), scale);

    renderNanovgFrame (nvg);
    
    nvgEndFrame (nvg);
}

void NanoVGComponent::shutdown()
{
    //nvgDeleteContext(nvg);
}


void NanoVGComponent::paintComponent()
{
    if (currentlyPainting)
        return;

    currentlyPainting = true;

    if (!isInitialised())
    {
        initialise();
    }

    const float width {getWidth() * scale};
    const float height {getHeight() * scale};

#if NANOVG_METAL
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
    setCachedComponentImage (new RenderCache (*this));
    attachTo (this);
    overlay.addToDesktop (juce::ComponentPeer::windowRepaintedExplictly);

#if NANOVG_GL2 || NANOVG_GL3 || NANOVG_GLES2 || NANOVG_GLES3 //turning this on improves linux framerate, but seems to expose thread safety issues on windows/mac. see git PRs #349 and #396
      openGLContext.setOpenGLVersionRequired(juce::OpenGLContext::openGL3_2);
      openGLContext.setContinuousRepainting(true);
      openGLContext.setComponentPaintingEnabled(false);
    
    
#endif
}

NanoVGComponent::~NanoVGComponent()
{
    detach();

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
#if JUCE_WINDOWS || JUCE_LINUX
    triggerAsyncUpdate();
#endif

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



void NanoVGComponent::resized()
{
    if (nvgGraphicsContext != nullptr)
    {
        nvgGraphicsContext->resized (int (getWidth() * scale), int (getHeight() * scale));
    }
}

void NanoVGComponent::renderNanovgFrame(NVGcontext* nvg)
{
    juce::Graphics g (*nvgGraphicsContext.get());
    paintEntireComponent (g, true);
}


NanoVGComponent::Overlay::Overlay()
{
    setOpaque (false);
    setInterceptsMouseClicks (true, true);
}

void NanoVGComponent::Overlay::setForwardComponent (juce::Component* comp) noexcept
{
    forwardComponent = comp;
}

juce::ComponentPeer* NanoVGComponent::Overlay::beginForward()
{
    if (!forwarding && forwardComponent != nullptr)
    {
        if (auto* peer = forwardComponent->getPeer())
        {
            // Prevent self-forwarding
            if (peer != getPeer())
            {
                forwarding = true;
                return peer;
            }
        }
    }

    return nullptr;
}

void NanoVGComponent::Overlay::endForward()
{
    jassert (forwarding);
    forwarding = false;
}

void NanoVGComponent::Overlay::forwardMouseEvent (const juce::MouseEvent& event)
{
    if (auto* peer {beginForward()})
    {
        peer->handleMouseEvent(event.source.getType(), event.position, event.mods, event.pressure,
                               event.orientation, event.eventTime.toMilliseconds());
        endForward();
    }
}

void NanoVGComponent::Overlay::forwardMouseWheelEvent (const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    if (auto* peer {beginForward()})
    {
        peer->handleMouseWheel (event.source.getType(), event.position, event.eventTime.toMilliseconds(), wheel);
        endForward();
    }
}

void NanoVGComponent::Overlay::forwardMouseMagnifyEvent (const juce::MouseEvent& event, float scaleFactor)
{
    if (auto* peer {beginForward()})
    {
        peer->handleMagnifyGesture (event.source.getType(), event.position, event.eventTime.toMilliseconds(), scaleFactor);
        endForward();
    }
}

void NanoVGComponent::Overlay::mouseMove (const juce::MouseEvent& event)
{
    forwardMouseEvent (event);
}

void NanoVGComponent::Overlay::mouseEnter (const juce::MouseEvent& event)
{
    forwardMouseEvent (event);
}

void NanoVGComponent::Overlay::mouseExit (const juce::MouseEvent& event)
{
    forwardMouseEvent (event);
}

void NanoVGComponent::Overlay::mouseDown (const juce::MouseEvent& event)
{
    forwardMouseEvent (event);
}

void NanoVGComponent::Overlay::mouseDrag (const juce::MouseEvent& event)
{
    forwardMouseEvent (event);
}

void NanoVGComponent::Overlay::mouseUp   (const juce::MouseEvent& event)
{
    forwardMouseEvent (event);
}

void NanoVGComponent::Overlay::mouseDoubleClick (const juce::MouseEvent& event)
{
    forwardMouseEvent (event);
}

void NanoVGComponent::Overlay::mouseWheelMove (const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    forwardMouseWheelEvent (event, wheel);
}

void NanoVGComponent::Overlay::mouseMagnify (const juce::MouseEvent& event, float scaleFactor)
{
    forwardMouseMagnifyEvent (event, scaleFactor);
}

//==============================================================================


void NanoVGComponent::attachTo (juce::Component* component)
{
    openGLContext.setMultisamplingEnabled(true);
    
    if (attachedComponent != nullptr)
    {
        detach();
        attachedComponent = nullptr;
    }

    if (component != nullptr)
    {
        attachedComponent = component;
        attachedComponent->addComponentListener (this);
#if NANOVG_METAL
        attachedComponent->addAndMakeVisible (embeddedView);
#elif NANOVG_GL2 || NANOVG_GL3 || NANOVG_GLES2 || NANOVG_GLES3
    attachedComponent->addAndMakeVisible (overlay);
    setOpaque (true);
    openGLContext.attachTo (*component);
    openGLContext.setContinuousRepainting (true);
    attachedComponent->addAndMakeVisible (overlay);
#endif
    overlay.setForwardComponent (attachedComponent);
    }
}

void NanoVGComponent::detach()
{
    if (attachedComponent != nullptr)
    {
        attachedComponent->removeComponentListener (this);
#if NANOVG_METAL
        attachedComponent->removeChildComponent (&embeddedView);
#endif
        attachedComponent = nullptr;
        overlay.setForwardComponent (nullptr);
        overlay.setVisible (false);
    }
}

void NanoVGComponent::setBackgroundColour (juce::Colour c)
{
    backgroundColour = c;
}

void NanoVGComponent::reset()
{
    if(nvg) {
        nvgReset(nvg);
    }
}

void NanoVGComponent::componentMovedOrResized (juce::Component& component, bool wasMoved, bool wasResized)
{
    if (!initialised)
        return;

    trackOverlay (wasMoved, wasResized);

    const auto width = uint32_t (scale * overlay.getWidth());
    const auto height = uint32_t (scale * overlay.getHeight());
    #if NANOVG_METAL
      mnvgSetViewBounds(embeddedView.getView(), width, height);
    #else
       //openGLContext.updateViewportSize(true);
    #endif

}

void NanoVGComponent::componentBeingDeleted (juce::Component& component)
{
    if (attachedComponent == &component)
        attachedComponent = nullptr;
}

void NanoVGComponent::repaintPeer()
{
#if JUCE_WINDOWS
    if (nativeWindow != nullptr)
        nativeWindow->repaint (juce::Rectangle<int> (0, 0, overlay.getWidth(), overlay.getHeight()));
#elif NANOVG_GL2 || NANOVG_GL3 || NANOVG_GLES2 || NANOVG_GLES3
    openGLContext.triggerRepaint();
#endif
}

void NanoVGComponent::trackOverlay (bool moved, bool resized)
{
    if (attachedComponent)
    {
        juce::Rectangle<int> bounds (0, 0, attachedComponent->getWidth(), attachedComponent->getHeight());
#if NANOVG_METAL
        embeddedView.setBounds (bounds);
#else
        
        juce::MessageManager::callAsync([this, bounds](){
            overlay.setBounds (bounds); // TODO: Do we need to do this?
        });

        //auto* topComp = overlay.getTopLevelComponent();

       // if (auto* peer = topComp->getPeer())
        //    updateWindowPosition (peer->getAreaCoveredBy (overlay));
#endif

        juce::MessageManager::callAsync([this](){
            //  Update scale for the current display
            if (auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
                scale = (float) display->scale;
        });


        if (resized)
            reset();
    }
}

#if JUCE_WINDOWS

void NanoVGComponent::updateWindowPosition (juce::Rectangle<int> bounds)
{
    if (nativeWindow != nullptr)
    {
        double nativeScaleFactor = 1.0;

        if (auto* peer = overlay.getTopLevelComponent()->getPeer())
            nativeScaleFactor = peer->getPlatformScaleFactor();

        if (! juce::approximatelyEqual (nativeScaleFactor, 1.0))
            bounds = (bounds.toDouble() * nativeScaleFactor).toNearestInt();

        SetWindowPos ((HWND) nativeWindow->getNativeHandle(), 0,
            bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(),
            SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER);
    }
}

#elif JUCE_LINUX

void NanoVGComponent::updateWindowPosition (juce::Rectangle<int> bounds)
{
    /*
    if (nativeWindow != nullptr)
    {
        double nativeScaleFactor = 1.0;

        if (auto* peer = overlay.getTopLevelComponent()->getPeer())
            nativeScaleFactor = peer->getPlatformScaleFactor();

        if (! juce::approximatelyEqual (nativeScaleFactor, 1.0))
            bounds = (bounds.toDouble() * nativeScaleFactor).toNearestInt();
    } */
}

#endif
