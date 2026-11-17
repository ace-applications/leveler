#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

// Leveler brand tokens (docs/ACE AUDIO Leveler.png)
namespace Lvl
{
    constexpr uint32_t kBg     = 0xff080808; // Obsidian Black
    constexpr uint32_t kWhite  = 0xffffffff; // Pure White
    constexpr uint32_t kGrey   = 0xffA3A3A3; // Neutral Grey
    constexpr uint32_t kPurple = 0xffB18CFF; // Electric Purple — live/active/chosen only
    constexpr uint32_t kGlass  = 0x08ffffff; // simulated glass fill
    constexpr uint32_t kHair   = 0x1fffffff; // hairline border
}

// Custom-painted rotary knob: 270° sweep, 0 dB at 12 o'clock, purple value arc.
class LevelKnob : public juce::Slider
{
public:
    LevelKnob();
    void paint (juce::Graphics&) override;
};

// Pill toggle: purple fill when on, hairline + grey text when off.
class PillToggle : public juce::ToggleButton
{
public:
    explicit PillToggle (const juce::String& label) : text (label) {}
    void setFont (juce::Typeface::Ptr t) noexcept { font = t; }
    void paint (juce::Graphics&) override;
private:
    juce::String text;
    juce::Typeface::Ptr font;
};

class LevelerAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit LevelerAudioProcessorEditor (LevelerAudioProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    LevelerAudioProcessor& proc;
    LevelKnob knob;
    juce::AudioProcessorValueTreeState::SliderAttachment levelAttachment {
        proc.apvts, "level", knob };
    PillToggle bypassButton { "BYPASS" };
    juce::AudioProcessorValueTreeState::ButtonAttachment bypassAttachment {
        proc.apvts, "bypass", bypassButton };
    juce::Label readout;

#if LEVELER_BUILD_PRO_DSP
    PillToggle autoButton { "AUTO" };
    juce::AudioProcessorValueTreeState::ButtonAttachment autoAttachment {
        proc.apvts, "auto", autoButton };
    juce::Label advisorTitle, advisorDirection, advisorExplanation, advisorDyn;
#endif

    juce::Typeface::Ptr isometra, inter, interSemi;
    juce::Image logo;
    bool knobWasAuto = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelerAudioProcessorEditor)
};
