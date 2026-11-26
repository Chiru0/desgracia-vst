#include "Params.h"

using APF = juce::AudioParameterFloat;
using APC = juce::AudioParameterChoice;
using API = juce::AudioParameterInt;
using APB = juce::AudioParameterBool;
using Attr = juce::AudioParameterFloatAttributes;

namespace
{
    juce::NormalisableRange<float> skewed (float lo, float hi, float centre)
    {
        juce::NormalisableRange<float> r (lo, hi);
        r.setSkewForCentre (centre);
        return r;
    }

    juce::String pct (float v, int)   { return juce::String (juce::roundToInt (v * 100.0f)) + "%"; }
    juce::String bip (float v, int)   { auto i = juce::roundToInt (v * 100.0f); return (i > 0 ? "+" : "") + juce::String (i) + "%"; }
    juce::String hz (float v, int)    { return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + "k" : juce::String (juce::roundToInt (v)) + "Hz"; }
    juce::String secs (float v, int)  { return v < 1.0f ? juce::String (juce::roundToInt (v * 1000.0f)) + "ms" : juce::String (v, 2) + "s"; }
    juce::String db (float v, int)    { return (v > 0 ? "+" : "") + juce::String (v, 1) + "dB"; }
    juce::String cents (float v, int) { auto i = juce::roundToInt (v); return (i > 0 ? "+" : "") + juce::String (i) + "ct"; }
    juce::String semis (float v, int) { return (v > 0 ? "+" : "") + juce::String (v, 1) + "st"; }
    juce::String rateHz (float v, int){ return v < 10.0f ? juce::String (v, 2) + "Hz" : juce::String (v, 1) + "Hz"; }

    template <typename T, typename... Args>
    void add (juce::AudioProcessorValueTreeState::ParameterLayout& l, Args&&... args)
    {
        l.add (std::make_unique<T> (std::forward<Args> (args)...));
    }

    void addFloat (juce::AudioProcessorValueTreeState::ParameterLayout& l, const juce::String& id, const juce::String& name,
                   juce::NormalisableRange<float> range, float def, juce::String (*fmt) (float, int))
    {
        l.add (std::make_unique<APF> (juce::ParameterID { id, 1 }, name, range, def,
                                      Attr().withStringFromValueFunction (fmt)));
    }

    void addOsc (juce::AudioProcessorValueTreeState::ParameterLayout& l, const char* tag,
                 const juce::String& w, const juce::String& o, const juce::String& s, const juce::String& f,
                 const juce::String& lv, const juce::String& sh, int defWave, float defLevel)
    {
        juce::String t (tag);
        add<APC> (l, juce::ParameterID { w, 1 }, "Osc " + t + " Wave", choices::waves, defWave);
        add<API> (l, juce::ParameterID { o, 1 }, "Osc " + t + " Octave", -3, 3, 0);
        add<API> (l, juce::ParameterID { s, 1 }, "Osc " + t + " Semi", -12, 12, 0);
        addFloat (l, f, "Osc " + t + " Fine", { -100.0f, 100.0f }, 0.0f, cents);
        addFloat (l, lv, "Osc " + t + " Level", { 0.0f, 1.0f }, defLevel, pct);
        addFloat (l, sh, "Osc " + t + " Shape", { 0.0f, 1.0f }, 0.0f, pct);
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;

    addOsc (l, "A", pid::oscAWave, pid::oscAOct, pid::oscASemi, pid::oscAFine, pid::oscALevel, pid::oscAShape, 2, 0.8f);
    addOsc (l, "B", pid::oscBWave, pid::oscBOct, pid::oscBSemi, pid::oscBFine, pid::oscBLevel, pid::oscBShape, 3, 0.0f);

    addFloat (l, pid::fmAmt, "FM B>A", { 0.0f, 1.0f }, 0.0f, pct);
    addFloat (l, pid::fmEnv, "FM Env", { 0.0f, 1.0f }, 0.0f, pct);
    addFloat (l, pid::ringAmt, "Ring Mod", { 0.0f, 1.0f }, 0.0f, pct);

    add<APC> (l, juce::ParameterID { pid::subWave, 1 }, "Sub Wave", choices::subWaves, 0);
    add<APC> (l, juce::ParameterID { pid::subOct, 1 }, "Sub Octave", choices::subOcts, 0);
    addFloat (l, pid::subLevel, "Sub Level", { 0.0f, 1.0f }, 0.0f, pct);
    addFloat (l, pid::noiseLevel, "Noise Level", { 0.0f, 1.0f }, 0.0f, pct);
    addFloat (l, pid::noiseColor, "Noise Color", { 0.0f, 1.0f }, 0.6f, pct);

    add<API> (l, juce::ParameterID { pid::uniVoices, 1 }, "Unison", 1, 7, 1);
    addFloat (l, pid::uniDetune, "Unison Detune", { 0.0f, 1.0f }, 0.2f, pct);
    addFloat (l, pid::uniSpread, "Unison Spread", { 0.0f, 1.0f }, 0.5f, pct);
    addFloat (l, pid::drift, "Drift", { 0.0f, 1.0f }, 0.1f, pct);

    add<APC> (l, juce::ParameterID { pid::fltType, 1 }, "Filter Type", choices::fltTypes, 1);
    addFloat (l, pid::fltCutoff, "Cutoff", skewed (20.0f, 20000.0f, 900.0f), 8000.0f, hz);
    addFloat (l, pid::fltReso, "Resonance", { 0.0f, 1.0f }, 0.1f, pct);
    addFloat (l, pid::fltDrive, "Filter Drive", { 0.0f, 1.0f }, 0.0f, pct);
    addFloat (l, pid::fltEnv, "Filter Env", { -1.0f, 1.0f }, 0.0f, bip);
    addFloat (l, pid::fltKey, "Key Track", { 0.0f, 1.0f }, 0.3f, pct);
    addFloat (l, pid::fltVel, "Filter Velocity", { 0.0f, 1.0f }, 0.2f, pct);

    auto envTime = skewed (0.0005f, 8.0f, 0.4f);
    addFloat (l, pid::ampA, "Amp Attack", envTime, 0.002f, secs);
    addFloat (l, pid::ampD, "Amp Decay", envTime, 0.4f, secs);
    addFloat (l, pid::ampS, "Amp Sustain", { 0.0f, 1.0f }, 0.8f, pct);
    addFloat (l, pid::ampR, "Amp Release", envTime, 0.15f, secs);
    addFloat (l, pid::ampVel, "Amp Velocity", { 0.0f, 1.0f }, 0.5f, pct);
    addFloat (l, pid::fenvA, "Filter Attack", envTime, 0.001f, secs);
    addFloat (l, pid::fenvD, "Filter Decay", envTime, 0.3f, secs);
    addFloat (l, pid::fenvS, "Filter Sustain", { 0.0f, 1.0f }, 0.0f, pct);
    addFloat (l, pid::fenvR, "Filter Release", envTime, 0.2f, secs);

    addFloat (l, pid::penvAmt, "Pitch Env", { -48.0f, 48.0f }, 0.0f, semis);
    addFloat (l, pid::penvDecay, "Pitch Env Decay", skewed (0.001f, 2.0f, 0.1f), 0.05f, secs);
    add<APC> (l, juce::ParameterID { pid::penvDest, 1 }, "Pitch Env Dest", choices::penvDests, 0);

    for (int i = 0; i < 2; ++i)
    {
        auto n = "LFO " + juce::String (i + 1) + " ";
        add<APC> (l, juce::ParameterID { pid::lfo (i, "shape"), 1 }, n + "Shape", choices::lfoShapes, 0);
        addFloat (l, pid::lfo (i, "rate"), n + "Rate", skewed (0.05f, 30.0f, 4.0f), i == 0 ? 5.5f : 0.5f, rateHz);
        add<APB> (l, juce::ParameterID { pid::lfo (i, "sync"), 1 }, n + "Sync", false);
        add<APC> (l, juce::ParameterID { pid::lfo (i, "div"), 1 }, n + "Division", choices::divisions, 4);
        add<APC> (l, juce::ParameterID { pid::lfo (i, "dest"), 1 }, n + "Dest", choices::lfoDests, i == 0 ? 0 : 1);
        addFloat (l, pid::lfo (i, "amt"), n + "Amount", { 0.0f, 1.0f }, 0.0f, pct);
        addFloat (l, pid::lfo (i, "delay"), n + "Delay", skewed (0.0f, 3.0f, 0.5f), 0.0f, secs);
    }

    add<APC> (l, juce::ParameterID { pid::voiceMode, 1 }, "Voice Mode", choices::voiceModes, 0);
    addFloat (l, pid::glide, "Glide", skewed (0.0f, 1.5f, 0.15f), 0.0f, secs);
    add<API> (l, juce::ParameterID { pid::bendRange, 1 }, "Bend Range", 1, 24, 2);

    add<APB> (l, juce::ParameterID { pid::drvOn, 1 }, "Drive On", false);
    add<APC> (l, juce::ParameterID { pid::drvType, 1 }, "Drive Type", choices::drvTypes, 0);
    addFloat (l, pid::drvAmt, "Drive", { 0.0f, 1.0f }, 0.3f, pct);
    addFloat (l, pid::drvMix, "Drive Mix", { 0.0f, 1.0f }, 1.0f, pct);

    add<APB> (l, juce::ParameterID { pid::crsOn, 1 }, "Crush On", false);
    addFloat (l, pid::crsBits, "Bits", { 2.0f, 16.0f }, 10.0f, [] (float v, int) { return juce::String (v, 1) + "bit"; });
    addFloat (l, pid::crsRate, "Sample Rate", skewed (500.0f, 44100.0f, 8000.0f), 16000.0f, hz);
    addFloat (l, pid::crsMix, "Crush Mix", { 0.0f, 1.0f }, 1.0f, pct);

    add<APB> (l, juce::ParameterID { pid::choOn, 1 }, "Chorus On", false);
    addFloat (l, pid::choRate, "Chorus Rate", skewed (0.05f, 8.0f, 1.0f), 0.6f, rateHz);
    addFloat (l, pid::choDepth, "Chorus Depth", { 0.0f, 1.0f }, 0.3f, pct);
    addFloat (l, pid::choMix, "Chorus Mix", { 0.0f, 1.0f }, 0.35f, pct);

    add<APB> (l, juce::ParameterID { pid::dlyOn, 1 }, "Delay On", false);
    add<APB> (l, juce::ParameterID { pid::dlySync, 1 }, "Delay Sync", true);
    add<APC> (l, juce::ParameterID { pid::dlyDiv, 1 }, "Delay Division", choices::divisions, 6);
    addFloat (l, pid::dlyTime, "Delay Time", skewed (10.0f, 1500.0f, 300.0f), 300.0f, [] (float v, int) { return juce::String (juce::roundToInt (v)) + "ms"; });
    addFloat (l, pid::dlyFb, "Delay Feedback", { 0.0f, 0.95f }, 0.35f, pct);
    addFloat (l, pid::dlyTone, "Delay Tone", skewed (300.0f, 18000.0f, 3000.0f), 4000.0f, hz);
    add<APB> (l, juce::ParameterID { pid::dlyPing, 1 }, "Ping Pong", true);
    addFloat (l, pid::dlyMix, "Delay Mix", { 0.0f, 1.0f }, 0.2f, pct);

    add<APB> (l, juce::ParameterID { pid::revOn, 1 }, "Reverb On", false);
    addFloat (l, pid::revSize, "Reverb Size", { 0.0f, 1.0f }, 0.5f, pct);
    addFloat (l, pid::revDamp, "Reverb Damping", { 0.0f, 1.0f }, 0.5f, pct);
    addFloat (l, pid::revMix, "Reverb Mix", { 0.0f, 1.0f }, 0.2f, pct);

    add<APB> (l, juce::ParameterID { pid::eqOn, 1 }, "EQ On", false);
    addFloat (l, pid::eqLow, "EQ Low", { -12.0f, 12.0f }, 0.0f, db);
    addFloat (l, pid::eqMid, "EQ Mid", { -12.0f, 12.0f }, 0.0f, db);
    addFloat (l, pid::eqMidFreq, "EQ Mid Freq", skewed (200.0f, 6000.0f, 1000.0f), 1000.0f, hz);
    addFloat (l, pid::eqHigh, "EQ High", { -12.0f, 12.0f }, 0.0f, db);

    addFloat (l, pid::width, "Stereo Width", { 0.0f, 2.0f }, 1.0f, pct);
    addFloat (l, pid::master, "Master", { -24.0f, 6.0f }, -3.0f, db);

    add<APB> (l, juce::ParameterID { pid::vocOn, 1 }, "Vocoder On", false);
    add<APC> (l, juce::ParameterID { pid::vocSrc, 1 }, "Vocoder Source", choices::vocSources, 0);
    add<APC> (l, juce::ParameterID { pid::vocPattern, 1 }, "Vocoder Words", choices::vocPatterns, 1);
    add<APC> (l, juce::ParameterID { pid::vocRate, 1 }, "Vocoder Rate", choices::divisions, 4);
    add<API> (l, juce::ParameterID { pid::vocBands, 1 }, "Vocoder Bands", 6, 24, 16);
    addFloat (l, pid::vocShift, "Vocoder Formant", { -12.0f, 12.0f }, 0.0f, semis);
    addFloat (l, pid::vocQ, "Vocoder Q", { 0.0f, 1.0f }, 0.5f, pct);
    addFloat (l, pid::vocSpeed, "Vocoder Speed", { 0.0f, 1.0f }, 0.5f, pct);
    addFloat (l, pid::vocSib, "Vocoder Sibilance", { 0.0f, 1.0f }, 0.3f, pct);
    addFloat (l, pid::vocMix, "Vocoder Mix", { 0.0f, 1.0f }, 1.0f, pct);

    addFloat (l, pid::macBright, "Macro Brillo", { -1.0f, 1.0f }, 0.0f, bip);
    addFloat (l, pid::macDirt, "Macro Mugre", { 0.0f, 1.0f }, 0.0f, pct);
    addFloat (l, pid::macDetune, "Macro Desafine", { 0.0f, 1.0f }, 0.0f, pct);
    addFloat (l, pid::macMove, "Macro Movimiento", { 0.0f, 1.0f }, 0.0f, pct);
    addFloat (l, pid::macSpace, "Macro Espacio", { 0.0f, 1.0f }, 0.0f, pct);
    addFloat (l, pid::macLength, "Macro Largo", { -1.0f, 1.0f }, 0.0f, bip);

    return l;
}
