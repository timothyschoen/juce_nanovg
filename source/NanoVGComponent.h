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
    class RenderCache : public CachedComponentImage,
                        private AsyncUpdater
    {
    public:
        RenderCache (NanoVGComponent& comp);
        ~RenderCache();

        void paint (Graphics&) override;
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

    void setBackgroundColour (Colour c);
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

    Colour backgroundColour {};
    
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
        void mouseMove        (const MouseEvent&) override;
        void mouseEnter       (const MouseEvent&) override;
        void mouseExit        (const MouseEvent&) override;
        void mouseDown        (const MouseEvent&) override;
        void mouseDrag        (const MouseEvent&) override;
        void mouseUp          (const MouseEvent&) override;
        void mouseDoubleClick (const MouseEvent&) override;
        void mouseWheelMove   (const MouseEvent&, const MouseWheelDetails&) override;
        void mouseMagnify     (const MouseEvent&, float scaleFactor) override;

    private:

        juce::ComponentPeer* beginForward();
        void endForward();
        void forwardMouseEvent (const MouseEvent&);
        void forwardMouseWheelEvent (const MouseEvent&, const MouseWheelDetails&);
        void forwardMouseMagnifyEvent (const MouseEvent&, float scaleFactor);

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

    NSViewComponent embeddedView;


    private:

    void trackOverlay (bool moved, bool resized);

    #if JUCE_WINDOWS
    void updateWindowPosition (juce::Rectangle<int> bounds);
    #endif

    bool initialised {false};
    float scale {1.0f};

    juce::Component* attachedComponent {nullptr};

    Overlay overlay{};
};

