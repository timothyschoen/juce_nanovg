//
//  Copyright (C) 2022 Arthur Benilov <arthur.benilov@gmail.com> and Timothy Schoen <timschoen123@gmail.com>
//


#include "NanoVGComponent.h"

#if JUCE_MAC
#include <nanovg_mtl.h>
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

void NanoVGComponent::paintComponent()
{
    if (currentlyPainting)
        return;

    currentlyPainting = true;

    if (!isInitialised())
    {
        if (getBounds().isEmpty())
        {
            // Component placement is not ready yet - postpone initialisation.
            currentlyPainting = false;
            return;
        }

        setBackgroundColour (backgroundColour);

        if (auto* display {Desktop::getInstance().getDisplays().getPrimaryDisplay()})
            scale = display->scale;

        if (attachedComponent == nullptr || initialised)
            return;

        scale = jmax (1.0f, scale);

        overlay.setVisible (true);

    #if JUCE_WINDOWS || JUCE_LINUX
        nativeWindow.reset (createNonRepaintingEmbeddedWindowsPeer (overlay,
                                                                    overlay.getTopLevelComponent()->getWindowHandle()));
        nativeWindow->setVisible (true);
    #endif

        auto* peer =
    #if JUCE_MAC
            overlay.getPeer();
    #elif JUCE_WINDOWS || JUCE_LINUX
            nativeWindow.get();
    #else
            nillptr;
    #endif

        jassert (peer != nullptr);


    #if JUCE_MAC
        
        embeddedView.setView (peer->getNativeHandle());
    #endif

        initialised = true;

        trackOverlay (false, true);

    #if JUCE_WINDOWS || JUCE_LINUX
        overlay.getTopLevelComponent()->repaint();
    #endif

    }

    const float width {getWidth() * scale};
    const float height {getHeight() * scale};

    
    renderFrame();

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
    overlay.addToDesktop (ComponentPeer::windowRepaintedExplictly);
}

NanoVGComponent::~NanoVGComponent()
{
    detach();
    
    if (nvg != nullptr)
    {
        nvgGraphicsContext->removeCachedImages();
        nvgDeleteContext(nvg);
        
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

void NanoVGComponent::RenderCache::paint (Graphics&)
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


void NanoVGComponent::renderFrame()
{
    if (!nvg)
    {
        const float width {getWidth() * scale};
        const float height {getHeight() * scale};
        
        nvgGraphicsContext.reset (new NanoVGGraphicsContext (embeddedView.getView(), (int)width, (int)height));
        nvg = nvgGraphicsContext->getContext();
        
        //mainFrameBuffer = nvgCreateFramebuffer(nvg, width, height, 0);
    }
    
    
    nvgBeginFrame (nvg, getWidth(), getHeight(), scale);

    renderNanovgFrame (nvg);

    nvgEndFrame (nvg);
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
    Graphics g (*nvgGraphicsContext.get());
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

void NanoVGComponent::Overlay::forwardMouseEvent (const MouseEvent& event)
{
    if (auto* peer {beginForward()})
    {
        peer->handleMouseEvent(event.source.getType(), event.position, event.mods, event.pressure,
                               event.orientation, event.eventTime.toMilliseconds());
        endForward();
    }
}

void NanoVGComponent::Overlay::forwardMouseWheelEvent (const MouseEvent& event, const MouseWheelDetails& wheel)
{
    if (auto* peer {beginForward()})
    {
        peer->handleMouseWheel (event.source.getType(), event.position, event.eventTime.toMilliseconds(), wheel);
        endForward();
    }
}

void NanoVGComponent::Overlay::forwardMouseMagnifyEvent (const MouseEvent& event, float scaleFactor)
{
    if (auto* peer {beginForward()})
    {
        peer->handleMagnifyGesture (event.source.getType(), event.position, event.eventTime.toMilliseconds(), scaleFactor);
        endForward();
    }
}

void NanoVGComponent::Overlay::mouseMove (const MouseEvent& event)
{
    forwardMouseEvent (event);
}

void NanoVGComponent::Overlay::mouseEnter (const MouseEvent& event)
{
    forwardMouseEvent (event);
}

void NanoVGComponent::Overlay::mouseExit (const MouseEvent& event)
{
    forwardMouseEvent (event);
}

void NanoVGComponent::Overlay::mouseDown (const MouseEvent& event)
{
    forwardMouseEvent (event);
}

void NanoVGComponent::Overlay::mouseDrag (const MouseEvent& event)
{
    forwardMouseEvent (event);
}

void NanoVGComponent::Overlay::mouseUp   (const MouseEvent& event)
{
    forwardMouseEvent (event);
}

void NanoVGComponent::Overlay::mouseDoubleClick (const MouseEvent& event)
{
    forwardMouseEvent (event);
}

void NanoVGComponent::Overlay::mouseWheelMove (const MouseEvent& event, const MouseWheelDetails& wheel)
{
    forwardMouseWheelEvent (event, wheel);
}

void NanoVGComponent::Overlay::mouseMagnify (const MouseEvent& event, float scaleFactor)
{
    forwardMouseMagnifyEvent (event, scaleFactor);
}

//==============================================================================


void NanoVGComponent::attachTo (juce::Component* component)
{
    if (attachedComponent != nullptr)
    {
        detach();
        attachedComponent = nullptr;
    }

    if (component != nullptr)
    {
        attachedComponent = component;
        attachedComponent->addComponentListener (this);
#if JUCE_MAC
        attachedComponent->addAndMakeVisible (embeddedView);
#elif JUCE_WINDOWS || JUCE_LINUX
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
#if JUCE_MAC
        attachedComponent->removeChildComponent (&embeddedView);
#endif
        attachedComponent = nullptr;
        overlay.setForwardComponent (nullptr);
        overlay.setVisible (false);
    }
}

void NanoVGComponent::setBackgroundColour (Colour c)
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
    
    mnvgSetViewBounds(embeddedView.getView(), width, height);
    
}

void NanoVGComponent::componentBeingDeleted (juce::Component& component)
{
    if (attachedComponent == &component)
        attachedComponent = nullptr;
}

void NanoVGComponent::repaintPeer()
{
#if JUCE_WINDOWS || JUCE_LINUX
    if (nativeWindow != nullptr)
        nativeWindow->repaint (juce::Rectangle<int> (0, 0, overlay.getWidth(), overlay.getHeight()));
#endif
}

void NanoVGComponent::trackOverlay (bool moved, bool resized)
{
    if (attachedComponent)
    {
        juce::Rectangle<int> bounds (0, 0, attachedComponent->getWidth(), attachedComponent->getHeight());
#if JUCE_MAC
        embeddedView.setBounds (bounds);
#elif JUCE_WINDOWS || JUCE_LINUX

        overlay.setBounds (bounds); // TODO: Do we need to do this?

        auto* topComp = overlay.getTopLevelComponent();

        if (auto* peer = topComp->getPeer())
            updateWindowPosition (peer->getAreaCoveredBy (overlay));
#endif

        if (moved)
        {
            // Update scale for the current display
            if (auto* display = Desktop::getInstance().getDisplays().getPrimaryDisplay())
                scale = (float) display->scale;
        }

        if (resized)
            reset();
    }
}

#if JUCE_WINDOWS || JUCE_LINUX

void NanoVGComponent::updateWindowPosition (juce::Rectangle<int> bounds)
{
    if (nativeWindow != nullptr)
    {
        double nativeScaleFactor = 1.0;

        if (auto* peer = overlay.getTopLevelComponent()->getPeer())
            nativeScaleFactor = peer->getPlatformScaleFactor();

        if (! approximatelyEqual (nativeScaleFactor, 1.0))
            bounds = (bounds.toDouble() * nativeScaleFactor).toNearestInt();

        SetWindowPos ((HWND) nativeWindow->getNativeHandle(), 0,
            bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(),
            SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER);
    }
}

#endif
