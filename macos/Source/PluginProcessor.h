#pragma once
#include <JuceHeader.h>
#include <atomic>

class GettoneSpacePannerProcessor : public juce::AudioProcessor
{
public:
    GettoneSpacePannerProcessor();
    ~GettoneSpacePannerProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState treeState;

    std::array<std::atomic<float>, 1024> scopeL{};
    std::array<std::atomic<float>, 1024> scopeR{};
    std::atomic<int> scopeIndex{ 0 };
    std::atomic<float> peakL{ 0.0f };
    std::atomic<float> peakR{ 0.0f };

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    std::array<juce::dsp::StateVariableTPTFilter<float>, 2> elevationFilters;
    juce::dsp::ProcessorChain<juce::dsp::StateVariableTPTFilter<float>, juce::dsp::Reverb> fxChain;

    juce::dsp::Limiter<float> safetyLimiter;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedX, smoothedY, smoothedZ;
    float currentSampleRate = 44100.0f;

private:
    // ... le tue variabili esistenti ...

// --- NUOVO: Buffer circolare custom per le Early Reflections ---
    juce::AudioBuffer<float> delayBuffer;
    int delayWritePosition = 0;

    // 8 Valori lisciati per le coordinate X e Y dei 4 punti di riflessione
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, 8> smoothedNodes;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GettoneSpacePannerProcessor)
};