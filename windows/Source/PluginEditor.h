#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class GettoneSpacePannerEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    GettoneSpacePannerEditor(GettoneSpacePannerProcessor&);
    ~GettoneSpacePannerEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override; // Aggiunto per rilasciare i nodi
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

    void timerCallback() override { repaint(); }

private:
    GettoneSpacePannerProcessor& audioProcessor;

    std::unique_ptr<juce::LookAndFeel_V4> hardwareLook;

    juce::ComboBox themeSelector;
    juce::ToggleButton bypassButton{ "BYPASS" };
    juce::ComboBox presetSelector;
    juce::TextButton savePresetButton{ "SAVE" };

    juce::Slider inGainSlider;
    juce::Slider outGainSlider;
    juce::Slider revSizeSlider;
    juce::Slider widthSlider;
    juce::ToggleButton sendModeButton{ "SEND MODE" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> themeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> revSizeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> widthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> sendModeAttachment;

    juce::File presetDir;
    std::unique_ptr<juce::FileChooser> fileChooser;

    // --- NUOVI COMPONENTI PER IL PANNELLO ADVANCED ---
    juce::TextButton advancedButton{ "ADVANCED" };
    bool isAdvancedPanelOpen = false;

    juce::Slider erLevelSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> erLevelAttachment;

    int draggedNodeIndex = -1;

    float randomOffsetX = 0.0f;
    float randomOffsetY = 0.0f;
    float randomRotation = 0.0f;

    juce::AffineTransform layoutTransform() const;
    void updateParametersFromMouse(juce::Point<float> mousePos);
    void refreshPresetList();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GettoneSpacePannerEditor)
};