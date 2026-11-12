#include "PluginEditor.h"
#include <BinaryData.h>
#include <cmath>

namespace
{
    juce::Colour c (uint32_t v) { return juce::Colour (v); }
    constexpr float pi = juce::MathConstants<float>::pi;
    juce::Font f (juce::Typeface::Ptr t, float h)
    { return juce::Font (juce::FontOptions().withTypeface (t).withHeight (h)); }
}

//==============================================================================
LevelKnob::LevelKnob()
{
    setRange (-12.0, 12.0, 0.5);
    setTextBoxStyle (NoTextBox, false, 0, 0);
    setSliderStyle (RotaryVerticalDrag);
}

void LevelKnob::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (6.0f);
    const float cx = bounds.getCentreX(), cy = bounds.getCentreY();
    const float radius = std::min (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const float arcR = radius - 8.0f;
    const float start = pi * 0.75f, end = pi * 2.25f; // 270° sweep, 0 dB at top
    const float frac = (float) ((getValue() + 12.0) / 24.0);
    const float angle = start + frac * (end - start);
    const float zero = start + 0.5f * (end - start);

    juce::Path track;
    track.addCentredArc (cx, cy, arcR, arcR, 0.0f, start, end, true);
    g.setColour (c (Lvl::kHair));
    g.strokePath (track, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // value arc from 0 dB to the current angle — purple only when the knob is live
    if (std::abs (getValue()) > 0.01)
    {
        juce::Path value;
        value.addCentredArc (cx, cy, arcR, arcR, 0.0f, zero, angle, frac < 0.5f);
        g.setColour (c (Lvl::kPurple));
        g.strokePath (value, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // center tick at 0 dB
    g.setColour (c (Lvl::kGrey).withAlpha (0.5f));
    g.drawEllipse (cx - 2.0f, cy - arcR - 2.0f, 4.0f, 4.0f, 1.0f);

    juce::Point<float> tip (cx + std::cos (angle) * (arcR - 10.0f), cy + std::sin (angle) * (arcR - 10.0f));
    juce::Point<float> back (cx + std::cos (angle) * (arcR - 26.0f), cy + std::sin (angle) * (arcR - 26.0f));
    g.setColour (c (Lvl::kWhite));
    g.drawLine ({ back, tip }, 3.0f);

    // simulated glass hub
    g.setColour (c (Lvl::kGlass));
    g.fillEllipse (cx - radius * 0.42f, cy - radius * 0.42f, radius * 0.84f, radius * 0.84f);
    g.setColour (c (Lvl::kHair));
    g.drawEllipse (cx - radius * 0.42f, cy - radius * 0.42f, radius * 0.84f, radius * 0.84f, 1.0f);
}

//==============================================================================
void PillToggle::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    if (getToggleState())
    {
        g.setColour (c (Lvl::kPurple));
        g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
        g.setColour (c (Lvl::kBg));
    }
    else
    {
        g.setColour (c (Lvl::kGlass));
        g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
        g.setColour (c (Lvl::kHair));
        g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.0f);
        g.setColour (c (Lvl::kGrey));
    }
    g.setFont (f (font, 12.0f));
    g.drawText (text, r, juce::Justification::centred);
}

//==============================================================================
LevelerAudioProcessorEditor::LevelerAudioProcessorEditor (LevelerAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    isometra  = juce::Typeface::createSystemTypefaceFor (BinaryData::IsometraRegular_ttf, BinaryData::IsometraRegular_ttfSize);
    inter     = juce::Typeface::createSystemTypefaceFor (BinaryData::InterRegular_ttf, BinaryData::InterRegular_ttfSize);
    interSemi = juce::Typeface::createSystemTypefaceFor (BinaryData::InterSemiBold_ttf, BinaryData::InterSemiBold_ttfSize);
    logo = juce::ImageCache::getFromMemory (BinaryData::logo_png, BinaryData::logo_pngSize);

    bypassButton.setFont (interSemi);
    addAndMakeVisible (knob);
    addAndMakeVisible (bypassButton);
    bypassButton.onClick = [&] { repaint(); };

    knob.onDragStart = [this]
    { // grabbing the knob while Auto is on returns control to manual (brief §6)
      #if LEVELER_BUILD_PRO_DSP
        if (proc.isAutoOn())
            *dynamic_cast<juce::AudioParameterBool*> (proc.apvts.getParameter ("auto")) = false;
      #endif
    };

  #if LEVELER_BUILD_PRO_DSP
    autoButton.setFont (interSemi);
    addAndMakeVisible (autoButton);
    autoButton.onClick = [&] { repaint(); };

    advisorTitle.setFont (f (interSemi, 11.0f));
    advisorTitle.setColour (juce::Label::textColourId, c (Lvl::kGrey));
    advisorTitle.setText ("ADVISOR", juce::dontSendNotification);
    advisorDirection.setFont (f (interSemi, 22.0f));
    advisorDirection.setJustificationType (juce::Justification::centredLeft);
    advisorDirection.setColour (juce::Label::textColourId, c (Lvl::kGrey));
    advisorExplanation.setFont (f (inter, 13.0f));
    advisorExplanation.setColour (juce::Label::textColourId, c (Lvl::kGrey));
    advisorDyn.setFont (f (inter, 11.0f));
    advisorDyn.setColour (juce::Label::textColourId, c (Lvl::kGrey).withAlpha (0.7f));
    addAndMakeVisible (advisorTitle);
    addAndMakeVisible (advisorDirection);
    addAndMakeVisible (advisorExplanation);
    addAndMakeVisible (advisorDyn);
  #endif

  #if LEVELER_BUILD_PRO_DSP
    setSize (420, 620);
  #else
    setSize (420, 420);
  #endif
    startTimerHz (30);
}

void LevelerAudioProcessorEditor::timerCallback()
{
  #if LEVELER_BUILD_PRO_DSP
    const bool autoOn = proc.isAutoOn();
    if (autoOn)
    {
        if (! knob.isMouseButtonDown())
            knob.setValue (proc.autoIntensity() * 12.0, juce::dontSendNotification); // knob follows engine
    }
    else
    {
        // Auto released: ease the displayed knob back to the manual parameter value
        const double manual = dynamic_cast<juce::AudioParameterFloat*> (proc.apvts.getParameter ("level"))->get();
        const double cur = knob.getValue();
        if (std::abs (cur - manual) > 0.05)
            knob.setValue (cur + (manual - cur) * 0.35, juce::dontSendNotification);
    }

    const auto r = proc.advisorResult();
    advisorDirection.setText (r.direction == Advisor::left ? "LEFT"
                              : r.direction == Advisor::right ? "RIGHT" : "NO CHANGE",
                              juce::dontSendNotification);
    advisorDirection.setColour (juce::Label::textColourId,
        r.direction == Advisor::none ? c (Lvl::kGrey) : c (Lvl::kPurple));
    advisorExplanation.setText (juce::String (r.explanation), juce::dontSendNotification);
    // NB: no String::formatted("%s") — on Windows that routes through swprintf and
    // mangles narrow char* into CJK mojibake
    const juce::String tier = r.dynDb < 12.0f ? "LOW" : r.dynDb < 18.0f ? "MODERATE" : "HIGH";
    advisorDyn.setText (tier + "  " + juce::String (r.dynDb, 1) + " dB",
                        juce::dontSendNotification);
    repaint();
  #endif
}

void LevelerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (c (Lvl::kBg));

    // title lockup: "Audio" white, "Leveler" purple — Isometra, title use only
    juce::AttributedString s;
    s.setJustification (juce::Justification::centred);
    s.append ("Audio ", f (isometra, 30.0f), c (Lvl::kWhite));
    s.append ("Leveler", f (isometra, 30.0f), c (Lvl::kPurple));
    s.draw (g, getLocalBounds().toFloat().removeFromTop (56.0f));

    // waveform logo (brand sheet asset), top-right of the title band
    if (logo.isValid())
        g.drawImageWithin (logo, getWidth() - 78, 12, 66, 34,
                           juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize,
                           false);

    // knob dB readout (purple while the engine drives it)
    const double v = knob.getValue();
    const juce::String readout = v == 0.0 ? "0.0 dB"
        : (v < 0 ? juce::String ("-") : juce::String ("+"))
              + juce::String (std::abs (v), 1) + " dB";
    const bool autoLive = proc.isAutoOn();
    g.setFont (f (interSemi, 18.0f));
    g.setColour (autoLive ? c (Lvl::kPurple) : c (Lvl::kWhite));
    g.drawText (readout, getLocalBounds().removeFromTop (312.0f).removeFromBottom (44.0f),
                juce::Justification::centred);

  #if LEVELER_BUILD_PRO_DSP
    if (autoLive)
    {
        g.setFont (f (interSemi, 11.0f));
        g.setColour (c (Lvl::kPurple));
        g.drawText ("AUTO MODE — GRAB KNOB TO OVERRIDE",
                    getLocalBounds().removeFromTop (330.0f).removeFromBottom (20.0f),
                    juce::Justification::centred);
    }

    // advisor confidence bar (under the panel)
    const auto r = proc.advisorResult();
    auto bar = juce::Rectangle<float> (60.0f, 546.0f, 300.0f, 4.0f);
    g.setColour (c (Lvl::kHair));
    g.fillRoundedRectangle (bar, 2.0f);
    if (r.direction != Advisor::none && r.confidence > 0.0f)
    {
        g.setColour (c (Lvl::kPurple));
        g.fillRoundedRectangle (bar.removeFromLeft (bar.getWidth() * r.confidence), 2.0f);
    }
  #endif
}

void LevelerAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop (56.0f); // title
    knob.setBounds (area.removeFromTop (248.0f).withSizeKeepingCentre (224, 224));

    area.removeFromTop (26.0f); // auto-mode hint strip (painted in paint())
    auto row = area.removeFromTop (52.0f).withSizeKeepingCentre (300, 32);
    bypassButton.setBounds (row.removeFromLeft (140));
  #if LEVELER_BUILD_PRO_DSP
    row.removeFromLeft (20);
    autoButton.setBounds (row.removeFromLeft (140));

    auto panel = area.removeFromTop (160.0f).reduced (32, 8);
    advisorTitle.setBounds (panel.removeFromTop (16.0f));
    advisorDirection.setBounds (panel.removeFromTop (28.0f));
    advisorDyn.setBounds (panel.removeFromTop (16.0f));
    advisorExplanation.setBounds (panel.removeFromTop (22.0f));
  #endif
}
