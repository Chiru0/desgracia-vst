#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>
#include "DSPCore.h"

// Plain snapshot of every synth parameter in real units.
// Filled once per audio block by the processor from APVTS atomics.
struct EngineParams
{
    struct OscP { int wave = 2, oct = 0, semi = 0; float fine = 0, level = 0.8f, shape = 0; };
    struct LfoP { int shape = 0; float rate = 1; bool sync = false; int div = 4, dest = 0; float amt = 0, delay = 0; };

    OscP a, b;
    float fm = 0, fmEnv = 0, ring = 0;
    int subWave = 0, subOct = 0; float subLevel = 0, noiseLevel = 0, noiseColor = 0.6f;
    int uniVoices = 1; float uniDetune = 0.2f, uniSpread = 0.5f, drift = 0.1f;

    int fltType = 1; float cutoff = 8000, reso = 0.1f, fltDrive = 0, fltEnv = 0, fltKey = 0.3f, fltVel = 0.2f;
    float ampA = 0.002f, ampD = 0.4f, ampS = 0.8f, ampR = 0.15f, ampVel = 0.5f;
    float fA = 0.001f, fD = 0.3f, fS = 0, fR = 0.2f;
    float penvAmt = 0, penvDecay = 0.05f; int penvDest = 0;
    LfoP lfo[2];
    int voiceMode = 0; float glide = 0; int bendRange = 2;
};

class SynthEngine
{
public:
    static constexpr int maxVoices = 16;
    static constexpr int maxUnison = 7;
    static constexpr int subBlock = 16;

    void prepare (double sampleRate);
    void reset();
    void setParams (const EngineParams& p) { params = p; }
    void setTransport (double bpm, bool playing, double ppq) { hostBpm = bpm > 1.0 ? bpm : 120.0; hostPlaying = playing; hostPpq = ppq; }

    // Renders (adds) into buffer, consuming MIDI sample-accurately.
    void render (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi);

    // quick fade-out of everything (used on preset change). Thread-safe.
    void requestPanic() { panicRequest = true; }

    // UI feedback (read from message thread)
    std::atomic<int>   noteOnCounter { 0 };
    std::atomic<int>   activeVoices { 0 };
    std::atomic<float> lastVelocity { 0.0f };
    std::atomic<int>   lastNote { 60 };

private:
    struct Voice
    {
        bool  active = false;
        int   note = -1;
        float velocity = 1.0f;
        bool  held = false, sustained = false;
        uint32_t age = 0;

        float pitch = 60.0f, targetPitch = 60.0f;   // semitones, for glide
        float penv = 0.0f;                          // pitch envelope (1 -> 0)
        float scoop = 0.0f;                         // "cheap synth" attack pitch slip
        float driftVal = 0.0f, driftTarget = 0.0f;
        float timeOn = 0.0f;                        // seconds since note on (LFO delay)
        float killGain = 1.0f; bool killing = false;
        float prevAmpMod = 1.0f;

        dsp::Env amp, fenv;
        std::array<dsp::Osc, maxUnison> oa, ob;
        std::array<float, maxUnison> uniOffset {}, uniDrift {}, panL {}, panR {};
        dsp::Osc sub;
        float noiseLp = 0.0f;
        dsp::SVF f1[2], f2[2], fv[3][2];
        float dcX[2] = { 0, 0 }, dcY[2] = { 0, 0 };
        dsp::Rng rng;
    };

    void noteOn (int note, float vel);
    void noteOff (int note);
    void allNotesOff (bool hard);
    void startVoice (Voice& v, int note, float vel, bool retrigger, bool resetPhases);
    Voice* findVoiceToUse (int note);
    void renderVoices (float* L, float* R, int n);
    void renderVoice (Voice& v, float* L, float* R, int n, const float* lfoVals);
    void handleMidi (const juce::MidiMessage& m);
    void updateLfos (int n);
    void releaseSustained();
    void softPanic();
    int lastVoiceMode = 0;
    std::atomic<bool> panicRequest { false };

    EngineParams params;
    std::array<Voice, maxVoices> voices;
    double sr = 44100.0;
    float  invSr = 1.0f / 44100.0f;
    uint32_t ageCounter = 0;
    float  lastPlayedPitch = 60.0f;

    // mono note stack
    int  stack[128] {}; int stackSize = 0;
    void stackPush (int n); void stackRemove (int n);

    bool  sustainPedal = false;
    float pitchBend = 0.0f;   // -1..1
    float modWheel = 0.0f;    // 0..1

    // LFO state (global, tempo-lockable)
    double lfoPhase[2] { 0, 0 };
    float  lfoSH[2] { 0, 0 }, lfoSHPrev[2] { 0, 0 }, lfoOut[2] { 0, 0 };
    double hostBpm = 120.0, hostPpq = 0.0; bool hostPlaying = false;
    double blockPpq = 0.0;
    dsp::Rng globalRng;

    // smoothed (per sub-block) continuous params
    struct Smooth { float v = 0; bool init = false; void step (float t, float c) { if (! init) { v = t; init = true; } else v += (t - v) * c; } };
    Smooth sCut, sReso, sLevA, sLevB, sShA, sShB, sFm, sRing, sSub, sNoise, sDrive;
    float  smoothCoef = 0.1f;

    juce::AudioBuffer<float> scratch;
};
