//
//  Copyright (C) 2022 Arthur Benilov <arthur.benilov@gmail.com>
//

#include "MainComponent.h"

MainComponent::MainComponent()
    :
#if NANOVG_GL_IMPLEMENTATION
label({}, "Drawing with NanoVG -> OpenGL almost works!"),
#elif NANOVG_GL_IMPLEMENTATION
label({}, "Drawing with NanoVG -> Metal works!"),
#endif
      button("Text button")
{
    setSize (600, 400);

    label.setJustificationType(juce::Justification::centred);
    //addAndMakeVisible(label);
    addAndMakeVisible(button);
    
    button.onClick = [this](){
        std::cout << "click!" << std::endl;
        
        getLookAndFeel().setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(rand() % 255, rand() % 255, rand() % 255));
        repaint();
    };

    startPeriodicRepaint(2);
}

MainComponent::~MainComponent()
{
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll(findColour(juce::ResizableWindow::backgroundColourId));
    
    g.setColour(juce::Colours::green);
    //g.fillRoundedRectangle(getLocalBounds().reduced(50).toFloat(), 3.0f);
}

void MainComponent::resized()
{
    auto bounds {getLocalBounds()};

    button.setBounds(bounds.removeFromBottom(80).reduced(20));
     label.setBounds(bounds.removeFromBottom(40));

}
