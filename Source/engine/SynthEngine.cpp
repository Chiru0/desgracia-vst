#include "SynthEngine.h"

using namespace dsp;

namespace
{
    inline double divBeats (int i)
    {
        static const double b[] = { 4.0, 2.0, 1.0, 2.0 / 3.0, 0.5, 1.0 / 3.0, 0.75, 0.25, 1.0 / 6.0, 0.125 };
        return b[juce::jlimit (0, 9, i)];
    }
    inline float semisToHz (float s) { return 440.0f * std::exp2 ((s - 69.0f) * (1.0f / 12.0f)); }

    struct Formant { float f[3]; float g[3]; };
    // A E I O U  (slightly "wrong", nasal-leaning formant sets)
    const Formant vowels[5] = {
        { { 800, 1150, 2900 }, { 1.0f, 0.55f, 0.25f } },
        { { 420, 1700, 2600 }, { 1.0f, 0.60f, 0.30f } },
        { { 290, 2250, 3050 }, { 1.0f, 0.45f, 0.35f } },
        { { 480,  820, 2800 }, { 1.0f, 0.70f, 0.15f } },
        { { 330,  680, 2500 }, { 1.0f, 0.45f, 0.12f } } };
}

void SynthEngine::prepare (double sampleRate)
{
    sr = sampleRate;
    invSr = (float) (1.0 / sampleRate);
    smoothCoef = 1.0f - std::exp (-(float) subBlock / (0.015f * (float) sr));
    uint32_t seed = 0xC0FFEEu;
    for (auto& v : voices)
    {
        v.amp.setSampleRate ((float) sr);
        v.fenv.setSampleRate ((float) sr);
        v.rng.seed (seed += 0x9E3779B9u);
    }
    globalRng.seed (0xDE5C4A1Au);
    reset();
}

void SynthEngine::reset()
{
    for (auto& v : voices)
    {
        v.active = false; v.held = v.sustained = false; v.note = -1;
        v.amp.kill(); v.fenv.kill();
        for (int c = 0; c < 2; ++c) { v.f1[c].reset(); v.f2[c].reset(); for (auto& f : v.fv) f[c].reset(); v.dcX[c] = v.dcY[c] = 0; }
    }
    stackSize = 0;
    sustainPedal = false;
    pitchBend = 0.0f;
    activeVoices = 0;
}

void SynthEngine::softPanic()
{
    for (auto& v : voices)
        if (v.active) v.killing = true;
    stackSize = 0;
}

//==============================================================================
void SynthEngine::stackPush (int n)
{
    stackRemove (n);
    if (stackSize < 128) stack[stackSize++] = n;
}

void SynthEngine::stackRemove (int n)
{
    for (int i = 0; i < stackSize; ++i)
        if (stack[i] == n)
        {
            for (int j = i; j < stackSize - 1; ++j) stack[j] = stack[j + 1];
            --stackSize;
            return;
        }
}

SynthEngine::Voice* SynthEngine::findVoiceToUse (int note)
{
    // 1) same note already sounding -> reuse (keeps phase continuity)
    for (auto& v : voices) if (v.active && v.note == note && ! v.killing) return &v;
    // 2) free voice
    for (auto& v : voices) if (! v.active) return &v;
    // 3) quietest released voice
    Voice* best = nullptr;
    for (auto& v : voices)
        if (! v.held && ! v.sustained && (best == nullptr || v.amp.level < best->amp.level)) best = &v;
    if (best) return best;
    // 4) oldest
    best = &voices[0];
    for (auto& v : voices) if (v.age < best->age) best = &v;
    return best;
}

void SynthEngine::startVoice (Voice& v, int note, float vel, bool retrigger, bool resetPhases)
{
    const auto& p = params;
    v.note = note;
    v.velocity = vel;
    v.held = true;
    v.sustained = false;
    v.age = ++ageCounter;
    v.killing = false; v.killGain = 1.0f;
    v.targetPitch = (float) note;

    if (! v.active)
        v.pitch = p.glide > 0.0005f ? lastPlayedPitch : (float) note;
    else if (p.voiceMode == 0 && p.glide <= 0.0005f)
        v.pitch = (float) note;

    if (resetPhases)
    {
        const int n = juce::jlimit (1, maxUnison, p.uniVoices);
        for (int u = 0; u < maxUnison; ++u)
        {
            float pa = n == 1 ? 0.0f : v.rng.unipolar();
            float pb = n == 1 ? 0.0f : v.rng.unipolar();
            v.oa[(size_t) u].reset (pa, v.rng.unipolar());
            v.ob[(size_t) u].reset (pb, v.rng.unipolar());
        }
        v.sub.reset (0.0f, 0.0f);
        v.noiseLp = 0.0f;
        for (int c = 0; c < 2; ++c) { v.f1[c].reset(); v.f2[c].reset(); for (auto& f : v.fv) f[c].reset(); v.dcX[c] = v.dcY[c] = 0; }
        v.driftVal = v.rng.bipolar(); v.driftTarget = v.rng.bipolar();
    }

    for (int u = 0; u < maxUnison; ++u) v.uniDrift[(size_t) u] = v.rng.bipolar();

    if (retrigger)
    {
        v.amp.noteOn();
        v.fenv.noteOn();
        v.penv = 1.0f;
        v.scoop = -p.drift * (0.25f + 0.25f * v.rng.unipolar());
        v.timeOn = 0.0f;
    }

    v.active = true;
    lastPlayedPitch = (float) note;
    lastNote = note;
    lastVelocity = vel;
    noteOnCounter.fetch_add (1);
}

void SynthEngine::noteOn (int note, float vel)
{
    if (params.voiceMode == 0)
    {
        auto* v = findVoiceToUse (note);
        startVoice (*v, note, vel, true, ! v->active);
        return;
    }

    // MONO / LEGATO: voice 0 only
    const bool othersHeld = stackSize > 0;
    stackPush (note);
    auto& v = voices[0];
    const bool legato = params.voiceMode == 2 && othersHeld && v.active && v.amp.stage != Env::Release;

    if (legato)
    {
        v.note = note; v.targetPitch = (float) note; v.held = true;
        if (params.glide <= 0.0005f) v.pitch = (float) note;
        lastPlayedPitch = (float) note; lastNote = note; lastVelocity = vel;
        noteOnCounter.fetch_add (1);
        return;
    }

    const bool wasActive = v.active;
    float prevPitch = v.pitch;
    startVoice (v, note, vel, true, ! wasActive);
    if (wasActive) v.pitch = params.glide > 0.0005f ? prevPitch : (float) note;
}

void SynthEngine::noteOff (int note)
{
    if (params.voiceMode == 0)
    {
        for (auto& v : voices)
            if (v.active && v.note == note && v.held)
            {
                v.held = false;
                if (sustainPedal) v.sustained = true;
                else { v.amp.noteOff(); v.fenv.noteOff(); }
            }
        return;
    }

    stackRemove (note);
    auto& v = voices[0];

    // safety: release any stray poly voices playing this note
    for (size_t i = 1; i < voices.size(); ++i)
        if (voices[i].active && voices[i].note == note) { voices[i].held = false; voices[i].amp.noteOff(); voices[i].fenv.noteOff(); }

    if (! v.active || v.note != note) return;

    if (stackSize > 0)
    {
        int back = stack[stackSize - 1];
        v.note = back; v.targetPitch = (float) back;
        if (params.glide <= 0.0005f) v.pitch = (float) back;
        lastPlayedPitch = (float) back;
        return;
    }

    v.held = false;
    if (sustainPedal) v.sustained = true;
    else { v.amp.noteOff(); v.fenv.noteOff(); }
}

void SynthEngine::releaseSustained()
{
    for (auto& v : voices)
        if (v.active && v.sustained && ! v.held)
        {
            v.sustained = false;
            v.amp.noteOff(); v.fenv.noteOff();
        }
}

void SynthEngine::allNotesOff (bool hard)
{
    for (auto& v : voices)
    {
        if (! v.active) continue;
        v.held = false; v.sustained = false;
        if (hard) v.killing = true;
        else { v.amp.noteOff(); v.fenv.noteOff(); }
    }
    stackSize = 0;
}

void SynthEngine::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn())                 noteOn (m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff())           noteOff (m.getNoteNumber());
    else if (m.isPitchWheel())        pitchBend = (float) (m.getPitchWheelValue() - 8192) / 8192.0f;
    else if (m.isSustainPedalOn())    sustainPedal = true;
    else if (m.isSustainPedalOff())   { sustainPedal = false; releaseSustained(); }
    else if (m.isAllSoundOff())       allNotesOff (true);
    else if (m.isAllNotesOff())       allNotesOff (false);
    else if (m.isController() && m.getControllerNumber() == 1) modWheel = (float) m.getControllerValue() / 127.0f;
}

//==============================================================================
void SynthEngine::updateLfos (int n)
{
    for (int i = 0; i < 2; ++i)
    {
        const auto& lp = params.lfo[i];
        double prev = lfoPhase[i];

        if (lp.sync)
        {
            const double beats = divBeats (lp.div);
            if (hostPlaying)
                lfoPhase[i] = std::fmod (blockPpq / beats, 1.0);
            else
                lfoPhase[i] += (double) n * invSr * (hostBpm / 60.0) / beats;
        }
        else
            lfoPhase[i] += (double) n * invSr * lp.rate;

        if (lfoPhase[i] >= 1.0) lfoPhase[i] -= std::floor (lfoPhase[i]);
        if (lfoPhase[i] < 0.0)  lfoPhase[i] += 1.0;
        const bool wrapped = lfoPhase[i] < prev;
        if (wrapped) { lfoSHPrev[i] = lfoSH[i]; lfoSH[i] = globalRng.bipolar(); }

        const float ph = (float) lfoPhase[i];
        float o = 0.0f;
        switch (lp.shape)
        {
            case 0: o = sin01 (ph); break;
            case 1: o = 1.0f - 4.0f * std::abs (ph - 0.5f); break;
            case 2: o = 1.0f - 2.0f * ph; break;
            case 3: o = juce::jlimit (-1.0f, 1.0f, sin01 (ph) * 6.0f); break;   // soft-edged square: no clicks on AMP
            case 4: o = lfoSH[i]; break;
            case 5: { float t = ph * ph * (3.0f - 2.0f * ph); o = lfoSHPrev[i] + (lfoSH[i] - lfoSHPrev[i]) * t; break; }
            default: break;
        }
        lfoOut[i] = o;
    }
}

void SynthEngine::render (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi)
{
    const int total = buffer.getNumSamples();
    float* L = buffer.getWritePointer (0);
    float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

    // voice mode changed: release everything so no voice is left orphaned
    static_assert (maxVoices > 1, "");
    if (params.voiceMode != lastVoiceMode)
    {
        allNotesOff (false);
        lastVoiceMode = params.voiceMode;
    }
    if (panicRequest.exchange (false)) softPanic();

    int pos = 0;
    auto it = midi.cbegin();
    while (pos < total)
    {
        while (it != midi.cend() && (*it).samplePosition <= pos)
        {
            handleMidi ((*it).getMessage());
            ++it;
        }
        int next = total;
        if (it != midi.cend()) next = juce::jmin (total, juce::jmax (pos + 1, (*it).samplePosition));
        int n = juce::jmin (subBlock, next - pos);

        blockPpq = hostPpq + (double) pos * invSr * hostBpm / 60.0;
        updateLfos (n);

        // smoothing of continuous params (once per sub-block)
        sCut.step (std::log2 (juce::jlimit (20.0f, 20000.0f, params.cutoff)), smoothCoef);
        sReso.step (params.reso, smoothCoef);
        sLevA.step (params.a.level, smoothCoef);
        sLevB.step (params.b.level, smoothCoef);
        sShA.step (params.a.shape, smoothCoef);
        sShB.step (params.b.shape, smoothCoef);
        sFm.step (params.fm, smoothCoef);
        sRing.step (params.ring, smoothCoef);
        sSub.step (params.subLevel, smoothCoef);
        sNoise.step (params.noiseLevel, smoothCoef);
        sDrive.step (params.fltDrive, smoothCoef);

        renderVoices (L + pos, R != nullptr ? R + pos : nullptr, n);
        pos += n;
    }
    // remaining events at the very end
    for (; it != midi.cend(); ++it) handleMidi ((*it).getMessage());

    int count = 0;
    for (auto& v : voices) if (v.active) ++count;
    activeVoices = count;
}

void SynthEngine::renderVoices (float* L, float* R, int n)
{
    float tmpR[subBlock];
    float* r = R != nullptr ? R : tmpR;
    for (auto& v : voices)
        if (v.active)
            renderVoice (v, L, r, n, lfoOut);
    if (R == nullptr)
        for (int i = 0; i < n; ++i) L[i] = 0.5f * (L[i] + tmpR[i]);
}

void SynthEngine::renderVoice (Voice& v, float* L, float* R, int n, const float* lfoVals)
{
    const auto& p = params;
    const float fn = (float) n;

    v.amp.setParams (p.ampA, p.ampD, p.ampS, p.ampR);
    v.fenv.setParams (p.fA, p.fD, p.fS, p.fR);

    // ---- pitch movement (glide, pitch env, scoop, drift)
    if (p.glide > 0.0005f)
        v.pitch += (v.targetPitch - v.pitch) * (1.0f - std::exp (-fn / (p.glide * 0.35f * (float) sr)));
    else
        v.pitch = v.targetPitch;

    v.penv  *= std::exp (-fn / (std::max (0.0005f, p.penvDecay) * 0.33f * (float) sr));
    v.scoop *= std::exp (-fn / (0.045f * (float) sr));
    if (v.rng.unipolar() < fn * invSr * 2.5f) v.driftTarget = v.rng.bipolar();
    v.driftVal += (v.driftTarget - v.driftVal) * (fn * invSr * 3.0f);
    v.timeOn += fn * invSr;

    // ---- LFO routing
    float lPitch = 0, lCut = 0, lShape = 0, lFm = 0, lPan = 0, lBPitch = 0, ampMod = 1.0f;
    for (int i = 0; i < 2; ++i)
    {
        const auto& lp = p.lfo[i];
        float amt = lp.amt;
        if (i == 0) amt = juce::jmin (1.0f, amt + modWheel * 0.35f);
        if (amt <= 0.0f) continue;
        if (lp.delay > 0.001f) amt *= juce::jmin (1.0f, v.timeOn / lp.delay);
        const float o = lfoVals[i];
        switch (lp.dest)
        {
            case 0: lPitch  += o * amt * amt * 12.0f; break;
            case 1: lCut    += o * amt * 4.0f; break;
            case 2: lShape  += o * amt * 0.5f; break;
            case 3: lFm     += (0.5f + 0.5f * o) * amt; break;
            case 4: ampMod  *= 1.0f - amt * (0.5f - 0.5f * o); break;
            case 5: lPan    += o * amt; break;
            case 6: lBPitch += o * amt * amt * 24.0f; break;
            default: break;
        }
    }

    const float bend = pitchBend * (float) p.bendRange;
    const float base = v.pitch + bend + v.scoop + p.drift * v.driftVal * 0.22f + lPitch;
    const float pe = p.penvAmt * v.penv;
    const float pitchA = base + (float) (p.a.oct * 12 + p.a.semi) + p.a.fine * 0.01f + (p.penvDest == 0 ? pe : 0.0f);
    const float pitchB = base + (float) (p.b.oct * 12 + p.b.semi) + p.b.fine * 0.01f + pe + lBPitch;

    const int nu = juce::jlimit (1, maxUnison, p.uniVoices);
    float dtA[maxUnison], dtB[maxUnison], gL[maxUnison], gR[maxUnison];
    const float uniGain = 1.0f / std::sqrt ((float) nu);
    for (int u = 0; u < nu; ++u)
    {
        const float off = nu > 1 ? ((float) u / (float) (nu - 1) * 2.0f - 1.0f) : 0.0f;
        const float det = off * p.uniDetune * 0.5f + v.uniDrift[(size_t) u] * p.drift * 0.10f;
        dtA[u] = juce::jlimit (0.0f, 0.49f, semisToHz (pitchA + det) * invSr);
        dtB[u] = juce::jlimit (0.0f, 0.49f, semisToHz (pitchB + det * 0.9f) * invSr);
        const float pan = juce::jlimit (-1.0f, 1.0f, off * p.uniSpread + lPan);
        gL[u] = std::sqrt (0.5f * (1.0f - pan)) * uniGain;
        gR[u] = std::sqrt (0.5f * (1.0f + pan)) * uniGain;
    }

    const float subPitch = v.pitch + bend + v.scoop - 12.0f * (float) (p.subOct + 1) + (p.penvDest == 0 ? pe : 0.0f);
    const float dtSub = juce::jlimit (0.0f, 0.49f, semisToHz (subPitch) * invSr);

    // ---- filter coefficients (once per sub-block)
    const float fenvNow = v.fenv.level;
    float oct = sCut.v + p.fltEnv * fenvNow * 7.0f + p.fltKey * (v.pitch - 60.0f) / 12.0f
                + p.fltVel * (v.velocity - 1.0f) * 2.5f + lCut;
    const float nyq = 0.45f * (float) sr;
    const float fc = juce::jlimit (20.0f, nyq, std::exp2 (oct));
    const float g = std::tan (pi * fc * invSr);
    const float res = sReso.v;
    const int ft = p.fltType;
    float vowelGain[3] = { 0, 0, 0 };
    if (ft == 5)
    {
        float posv = juce::jlimit (0.0f, 3.999f, (oct - 7.64f) / (12.29f - 7.64f) * 4.0f);
        int i0 = (int) posv; float t = posv - (float) i0;
        const float q = 3.0f + 17.0f * res, kk = 1.0f / q;
        for (int k = 0; k < 3; ++k)
        {
            float f = vowels[i0].f[k] + (vowels[i0 + 1].f[k] - vowels[i0].f[k]) * t;
            vowelGain[k] = vowels[i0].g[k] + (vowels[i0 + 1].g[k] - vowels[i0].g[k]) * t;
            float gg = std::tan (pi * juce::jmin (f, nyq) * invSr);
            v.fv[k][0].set (gg, kk); v.fv[k][1].set (gg, kk);
        }
    }
    else if (ft == 1)
    {
        v.f1[0].set (g, 2.0f - 1.1f * res); v.f1[1].set (g, 2.0f - 1.1f * res);
        v.f2[0].set (g, 2.0f - 1.93f * res); v.f2[1].set (g, 2.0f - 1.93f * res);
    }
    else
    {
        const float k = ft == 4 ? 2.0f - 1.6f * res : 2.0f - 1.96f * res;
        v.f1[0].set (g, k); v.f1[1].set (g, k);
    }

    const int wA = p.a.wave, wB = p.b.wave;
    const float shA = juce::jlimit (0.0f, 1.0f, sShA.v + lShape);
    const float shB = juce::jlimit (0.0f, 1.0f, sShB.v + lShape);
    const float levA = sLevA.v, levB = sLevB.v, ring = sRing.v;
    const float fmBase = sFm.v + lFm;
    const bool needB = levB > 0.0001f || fmBase > 0.0001f || p.fmEnv > 0.0001f || ring > 0.0001f;
    const float subLev = sSub.v, noiseLev = sNoise.v * 0.6f;
    const float noiseA = 0.03f + 0.97f * p.noiseColor * p.noiseColor;
    const float drv = sDrive.v, drvG = 1.0f + drv * 7.0f, drvComp = 1.0f / std::sqrt (drvG);
    const float velGain = 1.0f - p.ampVel * (1.0f - v.velocity);
    const float ampStart = v.prevAmpMod, ampStep = (ampMod - v.prevAmpMod) / fn;
    const float dcR = 1.0f - twoPi * 7.0f * invSr;
    const float killStep = 1.0f / (0.006f * (float) sr);

    for (int i = 0; i < n; ++i)
    {
        const float fe = v.fenv.tick();
        const float fmIdx = (fmBase + p.fmEnv * fe) * 0.9f;

        float sl = 0.0f, sr_ = 0.0f;
        for (int u = 0; u < nu; ++u)
        {
            float b = needB ? v.ob[(size_t) u].tick (wB, dtB[u], shB, 0.0f) : 0.0f;
            float a = v.oa[(size_t) u].tick (wA, dtA[u], shA, fmIdx > 0.0f ? fmIdx * b : 0.0f);
            float s = a * levA + b * levB + ring * a * b;
            sl += s * gL[u]; sr_ += s * gR[u];
        }

        float extra = 0.0f;
        if (subLev > 0.0001f)
        {
            int sw = p.subWave == 0 ? Sine : (p.subWave == 1 ? Tri : Pulse);
            extra += v.sub.tick (sw, dtSub, 0.0f, 0.0f) * subLev;
        }
        if (noiseLev > 0.0001f)
        {
            v.noiseLp += noiseA * (v.rng.bipolar() - v.noiseLp);
            extra += v.noiseLp * noiseLev;
        }
        sl += extra * 0.7071f; sr_ += extra * 0.7071f;

        float io[2] = { sl, sr_ };
        for (int c = 0; c < 2; ++c)
        {
            float x = io[c];
            if (drv > 0.001f) x = std::tanh (x * drvG) * drvComp * 1.4f;
            float lp, bp, hp, y = x;
            switch (ft)
            {
                case 0: v.f1[c].tick (x, lp, bp, hp); y = lp; break;
                case 1: v.f1[c].tick (x, lp, bp, hp); v.f2[c].tick (lp, lp, bp, hp); y = lp; break;
                case 2: v.f1[c].tick (x, lp, bp, hp); y = bp * v.f1[c].k * (1.0f + 1.5f * res); break;
                case 3: v.f1[c].tick (x, lp, bp, hp); y = hp; break;
                case 4: v.f1[c].tick (x, lp, bp, hp); y = lp + hp; break;
                case 5:
                {
                    y = 0.0f;
                    for (int k = 0; k < 3; ++k)
                    {
                        v.fv[k][c].tick (x, lp, bp, hp);
                        y += bp * v.fv[k][c].k * vowelGain[k];
                    }
                    y *= 2.2f;
                    break;
                }
                default: break;
            }
            // DC blocker
            float d = y - v.dcX[c] + dcR * v.dcY[c];
            v.dcX[c] = y; v.dcY[c] = d;
            io[c] = d;
        }

        float env = v.amp.tick() * velGain * (ampStart + ampStep * (float) i);
        if (v.killing)
        {
            v.killGain -= killStep;
            if (v.killGain <= 0.0f) { v.killGain = 0.0f; v.amp.kill(); }
            env *= v.killGain;
        }
        L[i] += io[0] * env * 0.7f;
        R[i] += io[1] * env * 0.7f;
    }
    v.prevAmpMod = ampMod;

    if (! v.amp.active())
    {
        v.active = false; v.held = false; v.sustained = false; v.killing = false; v.note = -1;
        v.fenv.kill();
    }
}
