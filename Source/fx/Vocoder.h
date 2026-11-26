#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include "engine/DSPCore.h"

struct VocoderParams
{
    bool on = false; int src = 0, pattern = 1, rateDiv = 4, bands = 16;
    float shift = 0, q = 0.5f, speed = 0.5f, sib = 0.3f, mix = 1.0f;
};

// Channel vocoder. Carrier = the synth. Modulator = either an internal
// "talking" formant voice (syllable patterns synced to the host tempo)
// or the plugin's sidechain input (your own voice / any audio).
class Vocoder
{
public:
    static constexpr int maxBands = 24;

    void prepare (double sampleRate);
    void reset();
    void retrigger() { stepPhase = 0.0; stepIndex = 0; newStep = true; }
    void process (float* L, float* R, const float* sideL, const float* sideR, int n,
                  const VocoderParams& p, double bpm, bool playing, double ppq);

    std::atomic<float> bandLevel[maxBands] {};
    std::atomic<int>   currentVowel { 0 };
    std::atomic<float> voiceGate { 0.0f };

private:
    void configure (int bands, float shift, float q);
    float voiceSample (const VocoderParams& p);

    double sr = 44100.0;
    float fade = 0.0f, fadeStep = 0.001f;

    dsp::SVF mod[maxBands], carL[maxBands], carR[maxBands];
    float env[maxBands] {};
    int cfgBands = -1; float cfgShift = 99.0f, cfgQ = -1.0f;

    // internal voice
    dsp::SVF formant[3];
    float vowelPos = 0.0f, vowelTarget = 0.0f;
    float buzzPhase = 0.0f, gate = 0.0f, burst = 0.0f, burstHp = 0.0f, burstLp = 0.0f;
    double stepPhase = 0.0; long long stepIndex = 0; bool newStep = true;
    double stepLenSamples = 10000.0;
    int curCons = 0;
    float sibLp = 0.0f;
    dsp::Rng rng;
};
