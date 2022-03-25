//
//  Copyright (C) 2022 Arthur Benilov <arthur.benilov@gmail.com> and Timothy Schoen <timschoen123@gmail.com>
//

#pragma once

#include <JuceHeader.h>

#include "NanoVGGraphics.h"

/**
    JUCE UI component rendered usin nanovg

    @note All normal JUCE components placed within this one will be
          rendered with nanovg as well.
*/

class MNVGframebuffer;

class NanoVGComponent : public juce::Component, public juce::ComponentListener, public juce::Timer
{
    class RenderCache : public juce::CachedComponentImage,
    private juce::AsyncUpdater
    {
    public:
        RenderCache (NanoVGComponent& comp);
        ~RenderCache();

        void paint (juce::Graphics&) override;
        bool invalidateAll() override;
        bool invalidate (const juce::Rectangle<int>&) override;
        void releaseResources() override;

    private:
        void handleAsyncUpdate() override;

        NanoVGComponent& component;
    };

    //------------------------------------------------------
public:
    
    NanoVGComponent();
    ~NanoVGComponent();
    
    bool isInitialised() const;

    void setBackgroundColour (juce::Colour c);
    void enableRenderStats();
    void startPeriodicRepaint(int fps = 30);
    void stopPeriodicRepaint();

    void renderFrame();



    virtual void renderNanovgFrame(NVGcontext* nvg);

    void resized() override;

private:
    
    friend class NanoVGComponent::RenderCache;

    void paintComponent();

    void timerCallback() override;

    bool currentlyPainting {false};
    bool showRenderStats {false};
    
    MNVGframebuffer* mainFrameBuffer = nullptr;

    NVGcontext* nvg {nullptr};
    std::unique_ptr<NanoVGGraphicsContext> nvgGraphicsContext {nullptr};

    juce::Colour backgroundColour {};
    
    /** Overlay component that will have nanovg attached to it.

        The overlay component will pe placed over the component
        that has nanovg context attached. It will forward mouse events
        down to its peer underneath.
    */
    
    class Overlay final : public juce::Component
    {
    public:
        Overlay();

        void setForwardComponent (juce::Component*) noexcept;

        // juce::Component
        void mouseMove        (const juce::MouseEvent&) override;
        void mouseEnter       (const juce::MouseEvent&) override;
        void mouseExit        (const juce::MouseEvent&) override;
        void mouseDown        (const juce::MouseEvent&) override;
        void mouseDrag        (const juce::MouseEvent&) override;
        void mouseUp          (const juce::MouseEvent&) override;
        void mouseDoubleClick (const juce::MouseEvent&) override;
        void mouseWheelMove   (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
        void mouseMagnify     (const juce::MouseEvent&, float scaleFactor) override;

    private:

        juce::ComponentPeer* beginForward();
        void endForward();
        void forwardMouseEvent (const juce::MouseEvent&);
        void forwardMouseWheelEvent (const juce::MouseEvent&, const juce::MouseWheelDetails&);
        void forwardMouseMagnifyEvent (const juce::MouseEvent&, float scaleFactor);

        juce::Component* forwardComponent {nullptr};
        bool forwarding {false};
    };

    //==========================================================================


    /** Attach the context to the given component. */
    void attachTo (juce::Component* component);

    /** Detach the context from the currently attached component. */
    void detach();

    float getScaleFactor() const noexcept { return scale; }

    void reset();

    void componentMovedOrResized (juce::Component& component, bool wasMoved, bool wasResized) override;
    void componentBeingDeleted (juce::Component& component) override;

    void repaintPeer();

    private:

    void trackOverlay (bool moved, bool resized);

    #if JUCE_WINDOWS || JUCE_LINUX
    void updateWindowPosition (juce::Rectangle<int> bounds);
    #endif
    
#if JUCE_MAC
    juce::NSViewComponent embeddedView;
#elif JUCE_WINDOWS || JUCE_LINUX
    std::unique_ptr<juce::ComponentPeer> nativeWindow {nullptr};
#else
#   error Unsupported platform
#endif

    bool initialised {false};
    float scale {1.0f};

    juce::Component* attachedComponent {nullptr};

    Overlay overlay{};
};

