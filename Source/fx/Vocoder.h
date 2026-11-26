#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include "engine/DSPCore.h"

struct VocoderParams
{
    bool on = false; int pattern = 0, rateDiv = 4, bands = 16;
    float shift = 0, q = 0.5f, speed = 0.5f, sib = 0.3f, mix = 1.0f;
};

// "Talking" filter (talkbox style). The synth goes through 3 moving formant
// filters that pronounce syllable patterns synced to the host tempo
// (wa-wa-wa, des-gra-cia, random robot noises...). There is no noise source:
// everything you hear is the synth itself, so it stays clean.
class Vocoder
{
public:
    static constexpr int maxBands = 24;

    void prepare (double sampleRate);
    void reset();
    void retrigger() { stepPhase = 0.0; stepIndex = 0; newStep = true; }
    void process (float* L, float* R, int n, const VocoderParams& p, double bpm, bool playing, double ppq);

    std::atomic<float> bandLevel[maxBands] {};   // for the UI visualiser
    std::atomic<int>   currentVowel { 0 };
    std::atomic<float> voiceGate { 0.0f };

private:
    void nextSyllable (const VocoderParams& p);
    void updateFormants (const VocoderParams& p);

    double sr = 44100.0;
    float fade = 0.0f, fadeStep = 0.001f;

    dsp::SVF fL[3], fR[3], consL, consR;
    float fFreq[3] { 800, 1150, 2900 }, fGain[3] { 1, 0.6f, 0.25f };
    float vowelPos = 0.0f, vowelTarget = 0.0f, gate = 0.0f, burst = 0.0f;
    float jump = 0.0f, jumpTarget = 0.0f;
    double stepPhase = 0.0; long long stepIndex = 0; bool newStep = true;
    double stepLenSamples = 10000.0;
    int curCons = 0, coefCounter = 0;
    float envIn = 0.0f, envOut = 0.0f, autoGain = 1.0f;
    dsp::Rng rng;
};
