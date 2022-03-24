//
//  Copyright (C) 2022 Arthur Benilov <arthur.benilov@gmail.com>
//

#include "MainComponent.h"

MainComponent::MainComponent()
    : label({}, "Drawing with NanoVG -> Metal works!"),
      button("Text button")
{
    setSize (600, 400);

    label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(label);
    addAndMakeVisible(button);

    //setBackgroundColour (Colour(0x1F, 0x1F, 0x1F));
    enableRenderStats();
    startPeriodicRepaint(2);
}

MainComponent::~MainComponent()
{
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll(Colours::rebeccapurple);
}

void MainComponent::resized()
{
    auto bounds {getLocalBounds()};

    button.setBounds(bounds.removeFromBottom(80).reduced(20));
     label.setBounds(bounds.removeFromBottom(40));

}
