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
    setSize (600, 400);

    label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(label);
    addAndMakeVisible(button);
    
    button.onClick = [this]()
    {
        getLookAndFeel().setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(rand() % 255, rand() % 255, rand() % 255));
        button.repaint();
    };
    
    startTimerHz(30);
}

MainComponent::~MainComponent()
{
}

void MainComponent::render()
{
    jassert(initialised);
    JUCE_ASSERT_MESSAGE_THREAD;

    const auto bgColour = findColour(juce::ResizableWindow::backgroundColourId);

#if NANOVG_GL_IMPLEMENTATION
    glViewport(0, 0, getWidth(), getHeight());
    glClearColor(bgColour.getRed(), bgColour.getGreen(), bgColour.getBlue(), bgColour.getAlpha());
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
#endif

    //nvgScissor(nvg, 0, 0, getWidth(), getHeight());
    auto* nvg = nvgGraphicsContext->getContext();
    nvgBeginFrame (nvg, getWidth(), getHeight(), 1.0f);

    juce::MessageManager::Lock mmLock;

    juce::Graphics g (*nvgGraphicsContext);
    paintEntireComponent (g, true);

#if NANOVG_GL_IMPLEMENTATION
    if (!openGLContext.isActive())
        openGLContext.makeActive();
// #elif NANOVG_METAL_IMPLEMENTATION
#endif
    NVGcolor nvgBGColour = nvgRGBA(bgColour.getRed(), bgColour.getGreen(), bgColour.getBlue(), bgColour.getAlpha());
    mnvgClearWithColor(nvg,nvgBGColour);

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
    
    nvgEndFrame (nvg);
    
    //openGLContext.swapBuffers();
}

void MainComponent::resized()
{
    NanoVGComponent::resized();
    auto bounds {getLocalBounds()};

    button.setBounds(bounds.removeFromBottom(80).reduced(20));
    label.setBounds(bounds.removeFromBottom(40));
}
