#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    juce::ignoreUnused (processorRef);

    if (auto* window = juce::TopLevelWindow::getTopLevelWindow(0))
    {
        window->setUsingNativeTitleBar(true);
    }

    // addChildComponent(mainComp);
    addChildComponent(demoComp);

    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (1000, 600);
    setResizable(true, true);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
}

//==============================================================================

void AudioPluginAudioProcessorEditor::resized()
{
    // mainComp.setBounds(getLocalBounds());
    demoComp.setBounds(0, 0, getWidth(), getHeight());
}
//==============================================================================
