#pragma once
#include <juce_dsp/juce_dsp.h>
#include "engine/DSPCore.h"

struct FxParams
{
    bool drvOn = false; int drvType = 0; float drvAmt = 0.3f, drvMix = 1.0f;
    bool crsOn = false; float crsBits = 10.0f, crsRate = 16000.0f, crsMix = 1.0f;
    bool choOn = false; float choRate = 0.6f, choDepth = 0.3f, choMix = 0.35f;
    bool dlyOn = false, dlySync = true, dlyPing = true; int dlyDiv = 6; float dlyTime = 300, dlyFb = 0.35f, dlyTone = 4000, dlyMix = 0.2f;
    bool revOn = false; float revSize = 0.5f, revDamp = 0.5f, revMix = 0.2f;
    bool eqOn = false; float eqLow = 0, eqMid = 0, eqMidFreq = 1000, eqHigh = 0;
    float width = 1.0f, masterDb = -3.0f;
};

// Order: DRIVE -> CRUSH -> EQ -> CHORUS -> DELAY -> REVERB -> WIDTH -> MASTER (soft ceiling)
// Every on/off switch is crossfaded, delay & reverb tails ring out when switched off.
class FxChain
{
public:
    void prepare (double sampleRate, int maxBlock);
    void reset();
    void process (juce::AudioBuffer<float>& buffer, const FxParams& p, double bpm);

private:
    struct Fade
    {
        float v = 0.0f, step = 0.001f;
        void prepare (double sr, float ms) { step = 1.0f / (float) (sr * ms * 0.001); }
        inline float next (bool on) { v = on ? std::min (1.0f, v + step) : std::max (0.0f, v - step); return v; }
        bool silent (bool on) const { return ! on && v <= 0.0f; }
    };

    double sr = 44100.0;
    int maxBlockSize = 512;

    // drive (2x oversampled)
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    float dcX[2] = { 0, 0 }, dcY[2] = { 0, 0 };
    Fade drvFade; juce::SmoothedValue<float> drvGain, drvMixS;

    // crush
    Fade crsFade; float crsAcc = 0.0f, crsHeld[2] = { 0, 0 }; juce::SmoothedValue<float> crsStepsS;

    // eq
    Fade eqFade; dsp::Biquad eqL, eqM, eqH; float eqCur[4] = { 99, 99, 99, 99 };

    // chorus
    Fade choFade; juce::dsp::Chorus<float> chorus; juce::AudioBuffer<float> choBuf;

    // delay
    Fade dlyFade; juce::AudioBuffer<float> dlyLine; int dlyWrite = 0; float dlySmoothed = 0.0f;
    float dlyLp[2] = { 0, 0 }, dlyHp[2] = { 0, 0 }; int dlyTail = 0;

    // reverb
    Fade revFade; juce::dsp::Reverb reverb; juce::AudioBuffer<float> revBuf; int revTail = 0;
    juce::dsp::Reverb::Parameters revParams;

    juce::SmoothedValue<float> widthS, masterS;
};
