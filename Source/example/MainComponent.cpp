//
//  Copyright (C) 2022 Arthur Benilov <arthur.benilov@gmail.com>
//

#include "MainComponent.h"

MainComponent::MainComponent()
    :
#if NANOVG_GL_IMPLEMENTATION
    label({}, "Drawing with NanoVG -> OpenGL almost works!"),
#elif NANOVG_METAL_IMPLEMENTATION
    label({}, "Drawing with NanoVG -> Metal works!"),
#endif
    button("Text button")
{
    label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(label);
    addAndMakeVisible(button);

    button.onClick = [this]()
    {
        // DBG("click!");

        auto colour = juce::Colour((juce::uint8)(rand() % 255), (juce::uint8)(rand() % 255), (juce::uint8)(rand() % 255));
        getLookAndFeel().setColour(juce::ResizableWindow::backgroundColourId, colour);
        button.repaint();
    };

    // startTimerHz(30);
}

MainComponent::~MainComponent()
{
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll(findColour(juce::ResizableWindow::backgroundColourId));

    if (auto* nvgContext = getNVGGraphicsContext())
    {
        auto* nvg = nvgContext->getContext();

        int x = 10, y = 10, w = 200, h = 200;
        float pos = 1.0f;

        float cy = y+(int)(h*0.5f);
        float kr = (int)(h*0.25f);

        NVGpaint knob = nvgLinearGradient(nvg, x,cy-kr,x,cy+kr, nvgRGBA(255,255,255,16), nvgRGBA(0,0,0,16));
        nvgBeginPath(nvg);
        nvgCircle(nvg, x+(int)(pos*w),cy, kr-1);
        nvgFillColor(nvg, nvgRGBA(40,43,48,255));
        nvgFill(nvg);
        nvgFillPaint(nvg, knob);
        nvgFill(nvg);

        nvgBeginPath(nvg);
        nvgCircle(nvg, x+(int)(pos*w),cy, kr-0.5f);
        nvgStrokeColor(nvg, nvgRGBA(0,0,0,92));
        nvgStroke(nvg);
    }
}

void MainComponent::resized()
{
    auto bounds {getLocalBounds()};

    button.setBounds(bounds.removeFromBottom(80).reduced(20));
    label.setBounds(bounds.removeFromBottom(40));
}
