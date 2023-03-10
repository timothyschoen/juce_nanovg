//
//  Copyright (C) 2022 Arthur Benilov <arthur.benilov@gmail.com> and Timothy Schoen <timschoen123@gmail.com>
//

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
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
    public juce::Timer
{
public:
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
    void paint(juce::Graphics&) override {}
    void resized() override;

protected:
    std::unique_ptr<NanoVGGraphicsContext> nvgGraphicsContext;
    std::atomic<bool> initialised = false;

private:
    void paintComponent();
    void timerCallback() override;

    bool currentlyPainting = false;
    bool showRenderStats = false;

    void initialise();
    virtual void render() = 0;
    void shutdown();

    //==========================================================================

    /** Detach the context from the currently attached component. */
    void detach();

#if JUCE_WINDOWS || JUCE_LINUX
    void updateWindowPosition (juce::Rectangle<int> bounds);
#endif
};
