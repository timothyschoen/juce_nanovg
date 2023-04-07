#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    juce::ignoreUnused (processorRef);
    setCachedComponentImage(nullptr);
    setBufferedToImage(false);

    if (auto* window = juce::TopLevelWindow::getTopLevelWindow(0))
    {
        window->setUsingNativeTitleBar(true);
    }

    addChildComponent(comp);

    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    getConstrainer()->setMaximumSize(2000, 1200);
    getConstrainer()->setMinimumSize(250, 150);
    setSize (1000, 600);
    setResizable(true, true);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
}

//==============================================================================

void AudioPluginAudioProcessorEditor::resized()
{
    comp.setBounds(getLocalBounds());
}
//==============================================================================
