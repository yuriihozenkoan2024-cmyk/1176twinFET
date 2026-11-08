#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

//==============================================================================
TwinFetProcessor::TwinFetProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "TwinFET", createLayout())
{
    pInput    = apvts.getRawParameterValue (pid::input);
    pOutput   = apvts.getRawParameterValue (pid::output);
    pAttack   = apvts.getRawParameterValue (pid::attack);
    pRelease  = apvts.getRawParameterValue (pid::release);
    pRatio    = apvts.getRawParameterValue (pid::ratio);
    pMode     = apvts.getRawParameterValue (pid::mode);
    pRevision = apvts.getRawParameterValue (pid::revision);
    pMix      = apvts.getRawParameterValue (pid::mix);
    pScHpf    = apvts.getRawParameterValue (pid::scHpf);
    pLink     = apvts.getRawParameterValue (pid::link);
    pCompOff  = apvts.getRawParameterValue (pid::compOff);
    pNoise    = apvts.getRawParameterValue (pid::noise);
    pOversampling = apvts.getRawParameterValue (pid::oversampling);

    if (! apvts.state.hasProperty (uiprop::advancedOpen)) apvts.state.setProperty (uiprop::advancedOpen, false, nullptr);
    if (! apvts.state.hasProperty (uiprop::showValues))   apvts.state.setProperty (uiprop::showValues, true, nullptr);

    startTimerHz (5);   // picks up oversampling changes on the message thread
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout TwinFetProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto dbText = [] (float v, int)
    {
        return (v > 0.04f ? String ("+") : String()) + String (v, 1) + " dB";
    };

    auto attackText = [] (float v, int)
    {
        const double us = 800.0 * std::pow (20.0 / 800.0, ((double) v - 1.0) / 6.0);
        const String t = us >= 1000.0 ? String (us / 1000.0, 2) + " ms"
                                      : String ((int) std::lround (us)) + String::fromUTF8 (" \xc2\xb5s");
        return String (v, 1) + "  (" + t + ")";
    };

    auto releaseText = [] (float v, int)
    {
        const double ms = 1100.0 * std::pow (50.0 / 1100.0, ((double) v - 1.0) / 6.0);
        const String t = ms >= 1000.0 ? String (ms / 1000.0, 2) + " s"
                                      : String ((int) std::lround (ms)) + " ms";
        return String (v, 1) + "  (" + t + ")";
    };

    auto percentText = [] (float v, int) { return String ((int) std::lround (v)) + " %"; };
    auto hpfText     = [] (float v, int) { return v <= 20.5f ? String ("Off") : String ((int) std::lround (v)) + " Hz"; };

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { pid::input, 1 }, "Input",
                    NormalisableRange<float> (-24.0f, 36.0f, 0.1f), 0.0f,
                    AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction (dbText)));

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { pid::output, 1 }, "Output",
                    NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f,
                    AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction (dbText)));

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { pid::attack, 1 }, "Attack",
                    NormalisableRange<float> (1.0f, 7.0f, 0.01f), 3.0f,
                    AudioParameterFloatAttributes().withStringFromValueFunction (attackText)));

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { pid::release, 1 }, "Release",
                    NormalisableRange<float> (1.0f, 7.0f, 0.01f), 5.0f,
                    AudioParameterFloatAttributes().withStringFromValueFunction (releaseText)));

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { pid::ratio, 1 }, "Ratio",
                    StringArray { "4:1", "8:1", "12:1", "20:1", "All" }, 0));

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { pid::mode, 1 }, "Mode",
                    StringArray { "Fat", "Clean", "Analog" }, 2));

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { pid::revision, 1 }, "Revision",
                    StringArray { "Black", "Blue" }, 0));

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { pid::mix, 1 }, "Mix",
                    NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f,
                    AudioParameterFloatAttributes().withLabel ("%").withStringFromValueFunction (percentText)));

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { pid::scHpf, 1 }, "Sidechain HPF",
                    NormalisableRange<float> (20.0f, 500.0f, 1.0f, 0.45f), 20.0f,
                    AudioParameterFloatAttributes().withLabel ("Hz").withStringFromValueFunction (hpfText)));

    layout.add (std::make_unique<AudioParameterBool> (ParameterID { pid::link, 1 }, "Stereo Link", true));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { pid::compOff, 1 }, "Compression Off", false));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { pid::noise, 1 }, "Analog Noise", false));

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { pid::meter, 1 }, "Meter",
                    StringArray { "GR", "+4", "+8", "Input" }, 0,
                    AudioParameterChoiceAttributes().withAutomatable (false)));

    // Auto = 4x up to 50 kHz, 2x up to 100 kHz, 1x above. Changing it changes the latency, so not automatable.
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { pid::oversampling, 1 }, "Oversampling",
                    StringArray { "Auto", "1x", "2x", "4x", "8x", "16x" }, 0,
                    AudioParameterChoiceAttributes().withAutomatable (false)));

    return layout;
}

//==============================================================================
twinfet::Params TwinFetProcessor::readParams() const
{
    twinfet::Params p;
    p.inputDb  = pInput->load();
    p.outputDb = pOutput->load();
    p.attack   = pAttack->load();
    p.release  = pRelease->load();
    p.ratio    = juce::roundToInt (pRatio->load());
    p.mode     = juce::roundToInt (pMode->load());
    p.revision = juce::roundToInt (pRevision->load());
    p.mix      = pMix->load() * 0.01f;
    const float hpf = pScHpf->load();
    p.scHpfHz  = hpf <= 20.5f ? 0.0f : hpf;
    p.link     = pLink->load() > 0.5f;
    p.compOff  = pCompOff->load() > 0.5f;
    p.noise    = pNoise->load() > 0.5f;
    return p;
}

//==============================================================================
void TwinFetProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const int channels = juce::jlimit (1, 2, getTotalNumOutputChannels());
    lastSampleRate = sampleRate;
    lastBlockSize  = samplesPerBlock;
    lastChannels   = channels;
    engineOsChoice = juce::roundToInt (pOversampling->load());
    engine.setOversamplingChoice (engineOsChoice);
    engine.setParams (readParams());
    engine.prepare (sampleRate, samplesPerBlock, channels);
    activeOversampling.store (engine.getOversamplingFactor());
    setLatencySamples (engine.getLatencySamples());

    for (auto& line : bypassLine)
        line.assign ((size_t) juce::jmax (1, engine.getLatencySamples()), 0.0f);
    bypassPos = 0;

    meterGrDb.store (0.0f);
    meterInMeanAbs.store (0.0f);
    meterOutMeanAbs.store (0.0f);
}

void TwinFetProcessor::reset()
{
    engine.reset();
}

//==============================================================================
void TwinFetProcessor::timerCallback()
{
    if (lastSampleRate <= 0.0)
        return;
    const int choice = juce::roundToInt (pOversampling->load());
    if (choice != engineOsChoice)
        applyOversampling (choice);
}

void TwinFetProcessor::applyOversampling (int choice)
{
    suspendProcessing (true);
    {
        const juce::ScopedLock sl (getCallbackLock());   // wait for a block that is still running
        engineOsChoice = choice;
        engine.setOversamplingChoice (choice);
        engine.setParams (readParams());
        engine.prepare (lastSampleRate, lastBlockSize, lastChannels);
        for (auto& line : bypassLine)
            line.assign ((size_t) juce::jmax (1, engine.getLatencySamples()), 0.0f);
        bypassPos = 0;
        activeOversampling.store (engine.getOversamplingFactor());
    }
    setLatencySamples (engine.getLatencySamples());
    suspendProcessing (false);
}

bool TwinFetProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

//==============================================================================
void TwinFetProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int totalIn    = getTotalNumInputChannels();
    const int totalOut   = getTotalNumOutputChannels();

    for (int ch = totalIn; ch < totalOut; ++ch)
        buffer.clear (ch, 0, numSamples);

    if (numSamples <= 0)
        return;

    engine.setParams (readParams());

    const int channels = juce::jlimit (1, 2, buffer.getNumChannels());
    engine.process (buffer.getArrayOfWritePointers(), channels, numSamples);

    // meter smoothing (~30 ms) so the editor's 60 Hz polling never aliases block sizes
    const double fs = getSampleRate() > 0.0 ? getSampleRate() : 48000.0;
    const float a = 1.0f - (float) std::exp (-(double) numSamples / (fs * 0.03));
    auto smooth = [a] (std::atomic<float>& m, float v)
    {
        const float old = m.load (std::memory_order_relaxed);
        m.store (old + a * (v - old), std::memory_order_relaxed);
    };
    smooth (meterGrDb, engine.getGainReductionDb());
    smooth (meterInMeanAbs, engine.getInputMeanAbs());
    smooth (meterOutMeanAbs, engine.getOutputMeanAbs());
}

void TwinFetProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    const int numSamples = buffer.getNumSamples();
    const int latency    = engine.getLatencySamples();
    const int channels   = juce::jmin (2, buffer.getNumChannels());

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    if (latency <= 0 || bypassLine[0].size() < (size_t) latency)
        return;

    int pos = bypassPos;
    for (int ch = 0; ch < channels; ++ch)
    {
        auto& line = bypassLine[(size_t) ch];
        float* data = buffer.getWritePointer (ch);
        pos = bypassPos;
        for (int i = 0; i < numSamples; ++i)
        {
            const float delayed = line[(size_t) pos];
            line[(size_t) pos] = data[i];
            data[i] = delayed;
            if (++pos >= latency) pos = 0;
        }
    }
    bypassPos = pos;
}

//==============================================================================
juce::AudioProcessorEditor* TwinFetProcessor::createEditor()
{
    return new TwinFetEditor (*this);
}

void TwinFetProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void TwinFetProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TwinFetProcessor();
}
