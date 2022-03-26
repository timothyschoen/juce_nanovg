//
//  Copyright (C) 2022 Arthur Benilov <arthur.benilov@gmail.com> and Timothy Schoen <timschoen123@gmail.com>
//

#pragma once

#include "NanoVGGraphics.h"

/**
    JUCE UI component rendered usin nanovg

    @note All normal JUCE components placed within this one will be
          rendered with nanovg as well.
*/

class MNVGframebuffer;

class NanoVGComponent :
#if NANOVG_METAL_IMPLEMENTATION
public juce::Component,
#elif NANOVG_GL_IMPLEMENTATION
public juce::OpenGLAppComponent,
#endif
public juce::ComponentListener, public juce::Timer
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

    void startPeriodicRepaint(int fps = 30);
    void stopPeriodicRepaint();

private:

    friend class NanoVGComponent::RenderCache;

    void paintComponent();

    void timerCallback() override;

    bool currentlyPainting {false};
    bool showRenderStats {false};

    MNVGframebuffer* mainFrameBuffer = nullptr;

    NVGcontext* nvg {nullptr};
    std::unique_ptr<NanoVGGraphicsContext> nvgGraphicsContext {nullptr};
    
    void initialise();
    void render();
    void shutdown();

    //==========================================================================

    /** Detach the context from the currently attached component. */
    void detach();

    void componentMovedOrResized (juce::Component& component, bool wasMoved, bool wasResized) override;

    private:

    #if JUCE_WINDOWS || JUCE_LINUX
    void updateWindowPosition (juce::Rectangle<int> bounds);
    #endif


    std::atomic<bool> initialised {false};
    float scale {1.0f};

    juce::Component* attachedComponent {nullptr};
};

