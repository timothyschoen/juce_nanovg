#pragma once
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
};
