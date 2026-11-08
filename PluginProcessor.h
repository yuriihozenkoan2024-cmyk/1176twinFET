#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <atomic>
#include <vector>

#include "TwinFetEngine.h"   // found via the include path (dsp/ or the project root)

namespace pid
{
    inline constexpr const char* input    = "input";
    inline constexpr const char* output   = "output";
    inline constexpr const char* attack   = "attack";
    inline constexpr const char* release  = "release";
    inline constexpr const char* ratio    = "ratio";
    inline constexpr const char* mode     = "mode";
    inline constexpr const char* revision = "revision";
    inline constexpr const char* mix      = "mix";
    inline constexpr const char* scHpf    = "schpf";
    inline constexpr const char* link     = "link";
    inline constexpr const char* compOff  = "compoff";
    inline constexpr const char* noise    = "noise";
    inline constexpr const char* meter    = "meter";
    inline constexpr const char* oversampling = "oversampling";
}

/** Editor-only settings stored with the plugin state (not automatable). */
namespace uiprop
{
    inline constexpr const char* advancedOpen = "uiAdvancedOpen";
    inline constexpr const char* showValues   = "uiShowValues";
}

//==============================================================================
class TwinFetProcessor final : public juce::AudioProcessor,
                               private juce::Timer
{
public:
    TwinFetProcessor();
    ~TwinFetProcessor() override { stopTimer(); }

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void reset() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    /** Host bypass: delay the dry signal by the reported latency so nothing shifts in time. */
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlockBypassed;

    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override                             { return 1; }
    int getCurrentProgram() override                          { return 0; }
    void setCurrentProgram (int) override                     {}
    const juce::String getProgramName (int) override          { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    //==========================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;

    // smoothed meter values for the editor (written on the audio thread)
    std::atomic<float> meterGrDb      { 0.0f };
    std::atomic<float> meterInMeanAbs { 0.0f };
    std::atomic<float> meterOutMeanAbs { 0.0f };

    /** Oversampling factor actually running (useful when the choice is "Auto"). */
    std::atomic<int> activeOversampling { 4 };

private:
    void timerCallback() override;
    void applyOversampling (int choice);   // message thread: re-prepares the engine while processing is suspended

    twinfet::Params readParams() const;

    twinfet::Engine engine;

    std::array<std::vector<float>, 2> bypassLine;
    int bypassPos = 0;

    std::atomic<float>* pInput    = nullptr;
    std::atomic<float>* pOutput   = nullptr;
    std::atomic<float>* pAttack   = nullptr;
    std::atomic<float>* pRelease  = nullptr;
    std::atomic<float>* pRatio    = nullptr;
    std::atomic<float>* pMode     = nullptr;
    std::atomic<float>* pRevision = nullptr;
    std::atomic<float>* pMix      = nullptr;
    std::atomic<float>* pScHpf    = nullptr;
    std::atomic<float>* pLink     = nullptr;
    std::atomic<float>* pCompOff  = nullptr;
    std::atomic<float>* pNoise    = nullptr;
    std::atomic<float>* pOversampling = nullptr;

    double lastSampleRate = 0.0;
    int lastBlockSize = 512, lastChannels = 2, engineOsChoice = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TwinFetProcessor)
};
