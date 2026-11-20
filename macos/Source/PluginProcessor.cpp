#include "PluginProcessor.h"
#include "PluginEditor.h"

GettoneSpacePannerProcessor::GettoneSpacePannerProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true).withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    treeState(*this, nullptr, "PARAMETERS", createParameterLayout())
#endif
{
}

GettoneSpacePannerProcessor::~GettoneSpacePannerProcessor() {}

const juce::String GettoneSpacePannerProcessor::getName() const { return "Gettone Space Panner"; }
bool GettoneSpacePannerProcessor::acceptsMidi() const { return false; }
bool GettoneSpacePannerProcessor::producesMidi() const { return false; }
bool GettoneSpacePannerProcessor::isMidiEffect() const { return false; }
double GettoneSpacePannerProcessor::getTailLengthSeconds() const { return 0.0; }
int GettoneSpacePannerProcessor::getNumPrograms() { return 1; }
int GettoneSpacePannerProcessor::getCurrentProgram() { return 0; }
void GettoneSpacePannerProcessor::setCurrentProgram(int) {}
const juce::String GettoneSpacePannerProcessor::getProgramName(int) { return {}; }
void GettoneSpacePannerProcessor::changeProgramName(int, const juce::String&) {}
void GettoneSpacePannerProcessor::releaseResources() {}

bool GettoneSpacePannerProcessor::isBusesLayoutSupported(const BusesLayout& layout) const
{
    return layout.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && layout.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}
bool GettoneSpacePannerProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* GettoneSpacePannerProcessor::createEditor() { return new GettoneSpacePannerEditor(*this); }

void GettoneSpacePannerProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = treeState.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void GettoneSpacePannerProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(treeState.state.getType()))
            treeState.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessorValueTreeState::ParameterLayout GettoneSpacePannerProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back(std::make_unique<juce::AudioParameterFloat>("posX", "Pos X", -1.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("posY", "Pos Y", -1.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("posZ", "Pos Z (Depth)", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("revSize", "Room Size", 0.1f, 1.5f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("width", "Stereo Width", 0.0f, 1.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("inGain", "In Gain", -24.0f, 24.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("outGain", "Out Gain", -24.0f, 24.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("bypass", "Bypass", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>("sendMode", "Send Mode", false));

    // 4 Punti di riflessione (Coordinate normalizzate da 0.0 a 1.0)
    params.push_back(std::make_unique<juce::AudioParameterFloat>("node1X", "Node 1 X", 0.0f, 1.0f, 0.1f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("node1Y", "Node 1 Y", 0.0f, 1.0f, 0.1f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("node2X", "Node 2 X", 0.0f, 1.0f, 0.9f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("node2Y", "Node 2 Y", 0.0f, 1.0f, 0.1f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("node3X", "Node 3 X", 0.0f, 1.0f, 0.1f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("node3Y", "Node 3 Y", 0.0f, 1.0f, 0.9f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("node4X", "Node 4 X", 0.0f, 1.0f, 0.9f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("node4Y", "Node 4 Y", 0.0f, 1.0f, 0.9f));

    // Livello mix delle Early Reflections
    params.push_back(std::make_unique<juce::AudioParameterFloat>("erLevel", "ER Level", 0.0f, 1.0f, 0.5f));

    juce::StringArray themes = { "Kawaii", "Dark Garage", "Tropical Beach", "Cosmic Void", "Vintage Wood Room" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>("theme", "Vibe", themes, 0));

    return { params.begin(), params.end() };
}

void GettoneSpacePannerProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = (float)sampleRate;
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();

    fxChain.prepare(spec);

    // Inizializza il nostro buffer custom (1 secondo di ritardo massimo in stereo)
    delayBuffer.setSize(getTotalNumOutputChannels(), (int)sampleRate + 2000);
    delayBuffer.clear();
    delayWritePosition = 0;

    for (auto& node : smoothedNodes) {
        node.reset(sampleRate, 0.15);
    }

    auto& airAbsorption = fxChain.template get<0>();
    airAbsorption.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    airAbsorption.setResonance(0.1f);

    for (auto& filter : elevationFilters) {
        filter.prepare(spec);
        filter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
        filter.setCutoffFrequency(juce::jmin(4500.0f, currentSampleRate * 0.49f));
        filter.setResonance(0.3f);
    }

    safetyLimiter.prepare(spec);
    safetyLimiter.setThreshold(-0.1f);
    safetyLimiter.setRelease(10.0f);

    smoothedX.reset(sampleRate, 0.15); smoothedY.reset(sampleRate, 0.15); smoothedZ.reset(sampleRate, 0.15);
}

void GettoneSpacePannerProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    bool isBypassed = treeState.getRawParameterValue("bypass")->load() > 0.5f;

    if (isBypassed) {
        juce::dsp::AudioBlock<float> audioBlock(buffer);
        juce::dsp::ProcessContextReplacing<float> context(audioBlock);
        safetyLimiter.process(context);
        peakL.store(buffer.getMagnitude(0, 0, buffer.getNumSamples()));
        if (buffer.getNumChannels() > 1) peakR.store(buffer.getMagnitude(1, 0, buffer.getNumSamples()));
        return;
    }

    float inGainDB = treeState.getRawParameterValue("inGain")->load();
    buffer.applyGain(juce::Decibels::decibelsToGain(inGainDB));

    float sizeMultiplier = treeState.getRawParameterValue("revSize")->load();
    float stereoWidth = treeState.getRawParameterValue("width")->load();
    int themeChoice = (int)treeState.getRawParameterValue("theme")->load();
    bool sendMode = treeState.getRawParameterValue("sendMode")->load() > 0.5f;

    // Caricamento del parametro erLevel
    float erLevel = treeState.getRawParameterValue("erLevel")->load();

    smoothedX.setTargetValue(treeState.getRawParameterValue("posX")->load());
    smoothedY.setTargetValue(treeState.getRawParameterValue("posY")->load());
    smoothedZ.setTargetValue(treeState.getRawParameterValue("posZ")->load());
    float currentZ = smoothedZ.getCurrentValue();

    smoothedNodes[0].setTargetValue(treeState.getRawParameterValue("node1X")->load());
    smoothedNodes[1].setTargetValue(treeState.getRawParameterValue("node1Y")->load());
    smoothedNodes[2].setTargetValue(treeState.getRawParameterValue("node2X")->load());
    smoothedNodes[3].setTargetValue(treeState.getRawParameterValue("node2Y")->load());
    smoothedNodes[4].setTargetValue(treeState.getRawParameterValue("node3X")->load());
    smoothedNodes[5].setTargetValue(treeState.getRawParameterValue("node3Y")->load());
    smoothedNodes[6].setTargetValue(treeState.getRawParameterValue("node4X")->load());
    smoothedNodes[7].setTargetValue(treeState.getRawParameterValue("node4Y")->load());

    // Puntatori per il nostro Delay Buffer Circolare
    int dBufSize = delayBuffer.getNumSamples();
    auto* dDataL = delayBuffer.getWritePointer(0);
    auto* dDataR = delayBuffer.getNumChannels() > 1 ? delayBuffer.getWritePointer(1) : dDataL;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        float curX = smoothedX.getNextValue();
        float curY = smoothedY.getNextValue();
        smoothedZ.getNextValue();

        float normSrcX = (curX + 1.0f) * 0.5f;
        float normSrcY = (curY + 1.0f) * 0.5f;

        float lDry = buffer.getSample(0, sample);
        float rDry = buffer.getSample(1, sample);

        // --- A. PANNING ORIGINALE ---
        float shelfGain = juce::jmap(curY, -1.0f, 1.0f, -1.0f, 1.5f);
        float lHigh = elevationFilters[0].processSample(1, lDry);
        float rHigh = elevationFilters[1].processSample(1, rDry);

        float theta = juce::MathConstants<float>::pi / 4.0f * (curX + 1.0f);
        float pannedL = (lDry + lHigh * shelfGain) * juce::dsp::FastMathApproximations::cos(theta);
        float pannedR = (rDry + rHigh * shelfGain) * juce::dsp::FastMathApproximations::sin(theta);

        // --- B. EARLY REFLECTIONS (Circular Buffer Custom) ---
        // Scriviamo l'audio nella posizione corrente della nostra "testina"
        dDataL[delayWritePosition] = pannedL;
        dDataR[delayWritePosition] = pannedR;

        float earlyRefL = 0.0f;
        float earlyRefR = 0.0f;

        for (int i = 0; i < 4; ++i)
        {
            float nodeX = smoothedNodes[i * 2].getNextValue();
            float nodeY = smoothedNodes[i * 2 + 1].getNextValue();

            float dx = normSrcX - nodeX;
            float dy = normSrcY - nodeY;
            float distance = std::sqrt(dx * dx + dy * dy);

            // Calcolo del ritardo
            float delayTimeMs = 5.0f + (distance * 40.0f) * sizeMultiplier;
            float delaySamples = delayTimeMs * (currentSampleRate / 1000.0f);

            float erDamping = 1.0f / (1.0f + sizeMultiplier * 1.5f);
            float bounceGain = (1.0f - juce::jlimit(0.0f, 1.0f, distance * 0.7f)) * erDamping;

            // Calcoliamo la posizione di lettura (andando a ritroso nel buffer)
            float readPos = delayWritePosition - delaySamples;
            if (readPos < 0.0f) readPos += dBufSize; // Wrap attorno al buffer

            // Interpolazione lineare per evitare click e artefatti
            int id1 = (int)readPos;
            int id2 = (id1 + 1) % dBufSize;
            float frac = readPos - id1;

            float tapL = dDataL[id1] + frac * (dDataL[id2] - dDataL[id1]);
            float tapR = dDataR[id1] + frac * (dDataR[id2] - dDataR[id1]);

            // Applicazione di erLevel al volume di uscita della riflessione
            earlyRefL += tapL * bounceGain * 0.25f * erLevel;
            earlyRefR += tapR * bounceGain * 0.25f * erLevel;
        }

        // Output al buffer principale
        buffer.setSample(0, sample, pannedL + earlyRefL);
        buffer.setSample(1, sample, pannedR + earlyRefR);

        // Avanziamo la testina e facciamo il loop se arriviamo alla fine
        delayWritePosition = (delayWritePosition + 1) % dBufSize;
    }

    // 5. REVERBERO LATE
    juce::dsp::Reverb::Parameters revParams;
    float baseSize = 0.8f; float damping = 0.45f;
    if (themeChoice == 1) { baseSize = 0.65f; damping = 0.85f; }
    else if (themeChoice == 2) { baseSize = 0.9f; damping = 0.35f; }
    else if (themeChoice == 3) { baseSize = 0.98f; damping = 0.1f; }
    else if (themeChoice == 4) { baseSize = 0.5f; damping = 0.95f; }

    revParams.roomSize = juce::jlimit(0.0f, 1.0f, baseSize * sizeMultiplier);
    revParams.damping = damping;
    revParams.width = stereoWidth;

    revParams.dryLevel = sendMode ? 0.0f : (1.0f - (std::pow(currentZ, 0.8f) * 0.85f));
    revParams.wetLevel = std::pow(currentZ, 1.2f) * 0.85f;
    fxChain.template get<1>().setParameters(revParams);

    // 6. LOWPASS ATMOSFERICO
    float minCutoffZ = 16000.0f;
    if (themeChoice == 1) minCutoffZ = 9000.0f;
    else if (themeChoice == 2) minCutoffZ = 13000.0f;
    else if (themeChoice == 3) minCutoffZ = 18000.0f;
    else if (themeChoice == 4) minCutoffZ = 4500.0f;

    const auto cutoff = juce::jmap(currentZ, 0.0f, 1.0f, 20000.0f, minCutoffZ);
    fxChain.template get<0>().setCutoffFrequency(juce::jmin(cutoff, currentSampleRate * 0.499f));

    juce::dsp::AudioBlock<float> audioBlock(buffer);
    juce::dsp::ProcessContextReplacing<float> context(audioBlock);
    fxChain.process(context);

    float outGainDB = treeState.getRawParameterValue("outGain")->load();
    buffer.applyGain(juce::Decibels::decibelsToGain(outGainDB));

    safetyLimiter.process(context);

    peakL.store(buffer.getMagnitude(0, 0, buffer.getNumSamples()));
    if (buffer.getNumChannels() > 1) peakR.store(buffer.getMagnitude(1, 0, buffer.getNumSamples()));

    int idx = scopeIndex.load(std::memory_order_relaxed);
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
        scopeL[(size_t) idx].store(buffer.getSample(0, sample), std::memory_order_relaxed);
        scopeR[(size_t) idx].store(buffer.getSample(1, sample), std::memory_order_relaxed);
        idx = (idx + 1) % 1024;
    }
    scopeIndex.store(idx, std::memory_order_release);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new GettoneSpacePannerProcessor(); }