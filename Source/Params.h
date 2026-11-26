#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// All parameter IDs live here so engine, presets and UI agree.
namespace pid
{
    // Oscillators
    inline const juce::String oscAWave  = "oscA_wave",  oscAOct  = "oscA_oct",  oscASemi = "oscA_semi",
                              oscAFine  = "oscA_fine",  oscALevel = "oscA_level", oscAShape = "oscA_shape";
    inline const juce::String oscBWave  = "oscB_wave",  oscBOct  = "oscB_oct",  oscBSemi = "oscB_semi",
                              oscBFine  = "oscB_fine",  oscBLevel = "oscB_level", oscBShape = "oscB_shape";
    inline const juce::String fmAmt = "fm_amt", fmEnv = "fm_env", ringAmt = "ring_amt";
    inline const juce::String subWave = "sub_wave", subOct = "sub_oct", subLevel = "sub_level";
    inline const juce::String noiseLevel = "noise_level", noiseColor = "noise_color";
    inline const juce::String uniVoices = "uni_voices", uniDetune = "uni_detune", uniSpread = "uni_spread", drift = "drift";

    // Filter
    inline const juce::String fltType = "flt_type", fltCutoff = "flt_cutoff", fltReso = "flt_reso", fltDrive = "flt_drive",
                              fltEnv = "flt_env", fltKey = "flt_key", fltVel = "flt_vel";

    // Envelopes
    inline const juce::String ampA = "amp_a", ampD = "amp_d", ampS = "amp_s", ampR = "amp_r", ampVel = "amp_vel";
    inline const juce::String fenvA = "fenv_a", fenvD = "fenv_d", fenvS = "fenv_s", fenvR = "fenv_r";
    inline const juce::String penvAmt = "penv_amt", penvDecay = "penv_decay", penvDest = "penv_dest";

    // LFOs (prefix lfo1_ / lfo2_)
    inline juce::String lfo (int i, const char* what) { return "lfo" + juce::String (i + 1) + "_" + what; }

    // Voice
    inline const juce::String voiceMode = "voice_mode", glide = "glide", bendRange = "bend_range";

    // FX
    inline const juce::String drvOn = "drv_on", drvType = "drv_type", drvAmt = "drv_amt", drvMix = "drv_mix";
    inline const juce::String crsOn = "crs_on", crsBits = "crs_bits", crsRate = "crs_rate", crsMix = "crs_mix";
    inline const juce::String choOn = "cho_on", choRate = "cho_rate", choDepth = "cho_depth", choMix = "cho_mix";
    inline const juce::String dlyOn = "dly_on", dlySync = "dly_sync", dlyDiv = "dly_div", dlyTime = "dly_time",
                              dlyFb = "dly_fb", dlyTone = "dly_tone", dlyPing = "dly_ping", dlyMix = "dly_mix";
    inline const juce::String revOn = "rev_on", revSize = "rev_size", revDamp = "rev_damp", revMix = "rev_mix";
    inline const juce::String eqOn = "eq_on", eqLow = "eq_low", eqMid = "eq_mid", eqMidFreq = "eq_midf", eqHigh = "eq_high";
    inline const juce::String width = "width", master = "master";
}

namespace choices
{
    inline const juce::StringArray waves      { "SINE", "TRI", "SAW", "PULSE", "TOY", "FORMANT", "BUZZ", "METAL" };
    inline const juce::StringArray subWaves   { "SINE", "TRI", "SQUARE" };
    inline const juce::StringArray subOcts    { "-1 OCT", "-2 OCT" };
    inline const juce::StringArray fltTypes   { "LP12", "LP24", "BP", "HP", "NOTCH", "VOWEL" };
    inline const juce::StringArray penvDests  { "ALL", "OSC B" };
    inline const juce::StringArray lfoShapes  { "SINE", "TRI", "SAW", "SQUARE", "S&H", "DRIFT" };
    inline const juce::StringArray lfoDests   { "PITCH", "CUTOFF", "SHAPE", "FM", "AMP", "PAN", "B PITCH" };
    inline const juce::StringArray divisions  { "1/1", "1/2", "1/4", "1/4T", "1/8", "1/8T", "1/8D", "1/16", "1/16T", "1/32" };
    inline const juce::StringArray voiceModes { "POLY", "MONO", "LEGATO" };
    inline const juce::StringArray drvTypes   { "SOFT", "HARD", "FOLD", "TUBE" };

    // length of each division in quarter notes (beats)
    inline double divisionBeats (int i)
    {
        static const double b[] = { 4.0, 2.0, 1.0, 2.0 / 3.0, 0.5, 1.0 / 3.0, 0.75, 0.25, 1.0 / 6.0, 0.125 };
        return b[juce::jlimit (0, 9, i)];
    }

    // what the SHAPE knob does for each waveform (shown in the UI)
    inline juce::String shapeName (int wave)
    {
        static const char* n[] = { "FOLD", "FOLD", "OCT+", "WIDTH", "STEPS", "FORMNT", "NARROW", "CLANG" };
        return n[juce::jlimit (0, 7, wave)];
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
