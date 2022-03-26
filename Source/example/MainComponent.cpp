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
        button.repaint();
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

    /*
    bool shouldDrawButtonAsHighlighted = false;
    bool shouldDrawButtonAsDown = false;
    
    auto cornerSize = 3.0f;
    auto bounds = button.getBounds().translated(0, -100).toFloat();

    g.setColour (findColour (juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle (bounds, cornerSize, 1.0f);
    
    g.setColour(findColour(juce::TextButton::buttonColourId));
    g.fillRoundedRectangle (bounds, cornerSize); */
    
    if(auto* nvgContext = dynamic_cast<const NanoVGGraphicsContext*>(&g.getInternalContext())){
        
        auto* nvg = nvgContext->getContext();
    }
    
}

void MainComponent::resized()
{
    auto bounds {getLocalBounds()};

    button.setBounds(bounds.removeFromBottom(80).reduced(20));
    label.setBounds(bounds.removeFromBottom(40));

}
