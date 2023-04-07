#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../NanoVGGraphics.h"

/**
 * Uses caching with the iGraphics startLayer() & endLayer()
*/
class CacheTest
    : public juce::Component
    , public NanoVGGraphics
{
public:
    CacheTest();
    ~CacheTest() override;

    void paint(juce::Graphics&) override;
    void drawCachable(NanoVGGraphics& g) override;
    void drawAnimated(NanoVGGraphics& g) override;
    void resized() override;
private:
    struct HoverTest
        : public ComponentLayer
        , public juce::Component
    {
        HoverTest();
        ~HoverTest() override {}
        void drawCachable(NanoVGGraphics& g) override;
        void drawAnimated(NanoVGGraphics& g) override;

        void mouseEnter (const juce::MouseEvent&) override;
        void mouseExit (const juce::MouseEvent&) override;
        void resized() override;

        bool mouseIsOver = false;
    };

    HoverTest childComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CacheTest)
};

//==============================================================================

/**
 * Binds & unbinds framebuffers in a neat RAII style way
 */
class ScopedFramebufferTest
    : public juce::Component
    , public NanoVGGraphics
{
public:
    ScopedFramebufferTest();
    ~ScopedFramebufferTest() override;

    // init framebuffer
    void contextCreated(NVGcontext*) override;

    void paint(juce::Graphics&) override;
    void draw(NanoVGGraphics& g) override;
    void resized() override;

private:
    Framebuffer framebuffer;
    int robotoFontId= -1;
};
