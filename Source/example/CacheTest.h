#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../NanoVGGraphics.h"

class CacheTest
    : public juce::Component
    , public NanoVGGraphics
{
public:
    CacheTest();
    ~CacheTest() override;

    void drawCachable(NanoVGGraphics& g) override;
    void drawAnimated(NanoVGGraphics& g) override;
    void resized() override;
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CacheTest)
};
