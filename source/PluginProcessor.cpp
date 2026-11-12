#include "PluginProcessor.h"
#include "PluginEditor.h"

static juce::AudioParameterFloat* levelParam (juce::AudioProcessorValueTreeState& s)
{ return dynamic_cast<juce::AudioParameterFloat*> (s.getParameter ("level")); }
static juce::AudioParameterBool* bypassParam (juce::AudioProcessorValueTreeState& s)
{ return dynamic_cast<juce::AudioParameterBool*> (s.getParameter ("bypass")); }
static juce::AudioParameterBool* autoParam (const juce::AudioProcessorValueTreeState& s)
{ return dynamic_cast<juce::AudioParameterBool*> (s.getParameter ("auto")); }

juce::AudioProcessorValueTreeState::ParameterLayout LevelerAudioProcessor::makeLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    l.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "level", 1 }, "Level",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.5f), 0.0f));
    l.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "bypass", 1 }, "Bypass", false));
  #if LEVELER_BUILD_PRO_DSP
    l.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "auto", 1 }, "Auto Mode", false));
  #endif
    return l;
}

LevelerAudioProcessor::LevelerAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", makeLayout())
{
    startTimer (250); // 4 Hz Advisor/Auto evaluation
}

LevelerAudioProcessor::~LevelerAudioProcessor() { stopTimer(); }

bool LevelerAudioProcessor::proAvailable() const
{
#if LEVELER_BUILD_PRO_DSP
    return LicenseState::proUnlocked();
#else
    return false;
#endif
}

bool LevelerAudioProcessor::isAutoOn() const
{
#if LEVELER_BUILD_PRO_DSP
    auto* p = autoParam (apvts);
    return proAvailable() && p != nullptr && p->get();
#else
    return false;
#endif
}

float LevelerAudioProcessor::autoIntensity() const
{
#if LEVELER_BUILD_PRO_DSP
    return autoMode.effectiveIntensity();
#else
    return 0.0f;
#endif
}

#if LEVELER_BUILD_PRO_DSP
Advisor::Result LevelerAudioProcessor::advisorResult() const { return advisor.result(); }
#endif

void LevelerAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    core.prepare (sampleRate, getTotalNumInputChannels());
    bypassGain.reset (sampleRate, 0.01); // 10 ms ramped true-bypass transitions
#if LEVELER_BUILD_PRO_DSP
    advisor.prepare (sampleRate);
    autoMode.setActive (false);
#endif
}

bool LevelerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    return in == out && (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo());
}

void LevelerAudioProcessor::timerCallback()
{
#if LEVELER_BUILD_PRO_DSP
    if (! proAvailable()) return;
    const bool on = autoParam (apvts)->get();
    autoMode.setActive (on);
    advisor.evaluate();
    autoMode.update (advisor.result(), juce::Time::getMillisecondCounterHiRes() * 0.001);
#endif
}

void LevelerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int numCh = buffer.getNumChannels();

    const bool bypassed = bypassParam (apvts)->get();
    bypassGain.setTargetValue (bypassed ? 0.0f : 1.0f);

#if LEVELER_BUILD_PRO_DSP
    // feed the advisor the linked-mono signal regardless of bypass state
    for (int i = 0; i < numSamples; ++i)
    {
        float e = 0.0f;
        for (int ch = 0; ch < numCh; ++ch) e += buffer.getSample (ch, i) * buffer.getSample (ch, i);
        advisor.processSample (std::sqrt (e / (float) juce::jmax (numCh, 1)));
    }
    const float n = autoParam (apvts)->get() ? autoMode.effectiveIntensity()
                                            : levelParam (apvts)->get() / 12.0f;
#else
    const float n = levelParam (apvts)->get() / 12.0f;
#endif

    const bool crossfading = bypassGain.isSmoothing();
    juce::AudioBuffer<float> dry;
    if (crossfading) dry.makeCopyOf (buffer, true);

    if (! bypassed || crossfading)
    {
        core.setIntensity (n);
        core.setAutoActive (isAutoOn());
        core.process (buffer.getArrayOfWritePointers(), numCh, numSamples);
    }

    if (crossfading)
        for (int i = 0; i < numSamples; ++i)
        {
            const float v = bypassGain.getNextValue(); // 1 = processed, 0 = bypassed
            for (int ch = 0; ch < numCh; ++ch)
            {
                const float p = buffer.getSample (ch, i);
                buffer.setSample (ch, i, dry.getSample (ch, i) * (1.0f - v) + p * v);
            }
        }
}

void LevelerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void LevelerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* LevelerAudioProcessor::createEditor() { return new LevelerAudioProcessorEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new LevelerAudioProcessor(); }
