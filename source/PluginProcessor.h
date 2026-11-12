#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/LevelerCore.h"
#if LEVELER_BUILD_PRO_DSP
    #include "dsp/Advisor.h"
    #include "dsp/AutoMode.h"
    #include "LicenseState.h"
#endif

class LevelerAudioProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    LevelerAudioProcessor();
    ~LevelerAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Leveler"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // UI surface (thread-safe)
    bool proAvailable() const;
    bool isAutoOn() const;
    float autoIntensity() const;
    #if LEVELER_BUILD_PRO_DSP
    Advisor::Result advisorResult() const;
    #endif

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout();
    void timerCallback() override; // 4 Hz Pro evaluation

    LevelerCore core;
    juce::SmoothedValue<float> bypassGain { 1.0f };

#if LEVELER_BUILD_PRO_DSP
    Advisor advisor;
    AutoMode autoMode;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelerAudioProcessor)
};
