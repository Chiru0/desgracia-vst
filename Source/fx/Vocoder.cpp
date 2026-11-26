#include "Vocoder.h"

namespace
{
    // A E I O U formants (F1 F2 F3) and gains
    const float vf[5][3] = { { 800, 1150, 2900 }, { 420, 1800, 2600 }, { 290, 2300, 3100 }, { 480, 820, 2800 }, { 330, 700, 2500 } };
    const float vg[5][3] = { { 1.0f, 0.6f, 0.25f }, { 1.0f, 0.55f, 0.3f }, { 1.0f, 0.5f, 0.35f }, { 1.0f, 0.7f, 0.15f }, { 1.0f, 0.45f, 0.12f } };

    struct Pattern { int len; int vowel[5]; int cons[5]; bool continuous; };
    const Pattern patterns[8] = {
        { 5, { 0, 1, 2, 3, 4 }, { 1, 1, 1, 1, 1 }, false },   // A-E-I-O-U
        { 4, { 1, 0, 2, 0, 0 }, { 1, 1, 1, 0, 0 }, false },   // DES-GRA-CI-A
        { 4, { 4, 0, 4, 0, 0 }, { 1, 0, 1, 0, 0 }, false },   // U-A U-A
        { 4, { 3, 2, 3, 2, 0 }, { 1, 1, 1, 1, 0 }, false },   // O-I O-I
        { 1, { 0, 0, 0, 0, 0 }, { 1, 0, 0, 0, 0 }, false },   // random
        { 1, { 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 }, true },    // AAAA
        { 1, { 4, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 }, true },    // WAH (morph)
        { 1, { 2, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 }, true } };  // IIII

    inline double divBeats (int i)
    {
        static const double b[] = { 4.0, 2.0, 1.0, 2.0 / 3.0, 0.5, 1.0 / 3.0, 0.75, 0.25, 1.0 / 6.0, 0.125 };
        return b[juce::jlimit (0, 9, i)];
    }
}

void Vocoder::prepare (double sampleRate)
{
    sr = sampleRate;
    fadeStep = (float) (1.0 / (sr * 0.03));
    rng.seed (0x0C0DE5u);
    cfgBands = -1;
    reset();
}

void Vocoder::reset()
{
    for (int i = 0; i < maxBands; ++i) { mod[i].reset(); carL[i].reset(); carR[i].reset(); env[i] = 0.0f; bandLevel[i] = 0.0f; }
    for (auto& f : formant) f.reset();
    gate = burst = burstHp = burstLp = sibLp = 0.0f;
    stepPhase = 0.0; stepIndex = 0; newStep = true;
}

void Vocoder::configure (int bands, float shift, float q)
{
    cfgBands = bands; cfgShift = shift; cfgQ = q;
    const float lo = 110.0f, hi = 7500.0f;
    const float ratio = std::pow (hi / lo, 1.0f / (float) (bands - 1));
    const float baseQ = 1.0f / (std::sqrt (ratio) - 1.0f / std::sqrt (ratio));
    const float Q = baseQ * (0.55f + 1.6f * q);
    const float k = 1.0f / Q;
    const float nyq = 0.45f * (float) sr;
    const float sh = std::exp2 (shift / 12.0f);
    for (int i = 0; i < bands; ++i)
    {
        const float f = lo * std::pow (ratio, (float) i);
        mod[i].set (std::tan (dsp::pi * juce::jmin (f, nyq) / (float) sr), k);
        const float g = std::tan (dsp::pi * juce::jmin (f * sh, nyq) / (float) sr);
        carL[i].set (g, k); carR[i].set (g, k);
    }
}

float Vocoder::voiceSample (const VocoderParams& p)
{
    const auto& pat = patterns[juce::jlimit (0, 7, p.pattern)];

    if (newStep)
    {
        newStep = false;
        if (p.pattern == 4) { vowelTarget = (float) (rng.nextU() % 5); curCons = (rng.nextU() & 1) ? 1 : 0; }
        else if (! pat.continuous) { const int s = (int) (stepIndex % pat.len); vowelTarget = (float) pat.vowel[s]; curCons = pat.cons[s]; }
        else vowelTarget = (float) pat.vowel[0];
        if (! pat.continuous && curCons) burst = 1.0f;
        currentVowel = (int) vowelTarget;
    }

    const float t = (float) stepPhase;   // 0..1 within the syllable
    if (p.pattern == 6)                  // WAH: U -> A -> U across two steps
    {
        const float w = 0.5f - 0.5f * std::cos (dsp::twoPi * ((float) (stepIndex % 2) + t) * 0.5f);
        vowelPos += ((4.0f * (1.0f - w)) - vowelPos) * 0.002f;
    }
    else
        vowelPos += (vowelTarget - vowelPos) * 0.0025f;

    // syllable gate: on, hold, short gap at the end of each step
    const float gTarget = pat.continuous ? 1.0f : (t < 0.82f ? 1.0f : 0.0f);
    gate += (gTarget - gate) * (gTarget > gate ? 0.004f : 0.0015f);
    voiceGate = gate;

    // glottal buzz + breath
    buzzPhase += 132.0f / (float) sr;
    if (buzzPhase >= 1.0f) buzzPhase -= 1.0f;
    const float src = (2.0f * buzzPhase - 1.0f) * 0.8f + rng.bipolar() * 0.12f;

    // formants
    const int i0 = juce::jlimit (0, 3, (int) vowelPos);
    const float fr = juce::jlimit (0.0f, 1.0f, vowelPos - (float) i0);
    float y = 0.0f;
    for (int k = 0; k < 3; ++k)
    {
        const float f = vf[i0][k] + (vf[i0 + 1][k] - vf[i0][k]) * fr;
        const float g = vg[i0][k] + (vg[i0 + 1][k] - vg[i0][k]) * fr;
        formant[k].set (std::tan (dsp::pi * f / (float) sr), 0.12f);
        float lp, bp, hp;
        formant[k].tick (src, lp, bp, hp);
        y += bp * 0.12f * g;
    }
    y *= gate * 2.5f;

    // consonant: short bright noise burst at the syllable start
    if (burst > 0.0005f)
    {
        const float nz = rng.bipolar();
        burstLp += 0.5f * (nz - burstLp);
        y += (nz - burstLp) * burst * 0.5f;
        burst *= 1.0f - 1.0f / (0.018f * (float) sr);
    }
    return y;
}

void Vocoder::process (float* L, float* R, const float* sideL, const float* sideR, int n,
                       const VocoderParams& p, double bpm, bool playing, double ppq)
{
    if (! p.on && fade <= 0.0f) return;

    const int nb = juce::jlimit (6, maxBands, p.bands);
    if (nb != cfgBands || std::abs (p.shift - cfgShift) > 0.01f || std::abs (p.q - cfgQ) > 0.001f)
        configure (nb, p.shift, p.q);

    const float atk = 1.0f - std::exp (-1.0f / ((0.0015f + (1.0f - p.speed) * 0.012f) * (float) sr));
    const float rel = 1.0f - std::exp (-1.0f / ((0.012f + (1.0f - p.speed) * 0.11f) * (float) sr));
    const double beats = divBeats (p.rateDiv);
    stepLenSamples = beats * 60.0 / juce::jmax (30.0, bpm) * sr;
    const bool internal = p.src == 0 || sideL == nullptr;
    const float makeup = 9.0f * std::sqrt ((float) nb / 16.0f) * (internal ? 1.0f : 1.6f);
    const float sibA = 1.0f - std::exp (-dsp::twoPi * 6000.0f / (float) sr);

    for (int i = 0; i < n; ++i)
    {
        fade = p.on ? juce::jmin (1.0f, fade + fadeStep) : juce::jmax (0.0f, fade - fadeStep);

        // step clock (locked to the host when playing)
        if (playing)
        {
            const double pos = (ppq + (double) i / sr * bpm / 60.0) / beats;
            const long long idx = (long long) std::floor (pos);
            if (idx != stepIndex) { stepIndex = idx; newStep = true; }
            stepPhase = pos - std::floor (pos);
        }
        else
        {
            stepPhase += 1.0 / stepLenSamples;
            if (stepPhase >= 1.0) { stepPhase -= 1.0; ++stepIndex; newStep = true; }
        }

        float m;
        if (internal) m = voiceSample (p);
        else m = 0.5f * (sideL[i] + (sideR != nullptr ? sideR[i] : sideL[i]));

        const float cl = L[i], cr = R[i];
        float ol = 0.0f, orr = 0.0f;
        for (int b = 0; b < nb; ++b)
        {
            float lp, bp, hp;
            mod[b].tick (m, lp, bp, hp);
            const float a = std::abs (bp * mod[b].k);
            env[b] += (a - env[b]) * (a > env[b] ? atk : rel);
            carL[b].tick (cl, lp, bp, hp); ol += bp * carL[b].k * env[b];
            carR[b].tick (cr, lp, bp, hp); orr += bp * carR[b].k * env[b];
        }
        // sibilance: let the modulator's top end through as noise
        const float hf = env[nb - 1] + env[nb - 2];
        sibLp += sibA * (rng.bipolar() - sibLp);
        const float s = (rng.bipolar() - sibLp) * hf * p.sib * 1.5f;

        const float wl = (ol + s) * makeup, wr = (orr + s) * makeup;
        const float m2 = p.mix * fade;
        L[i] = cl + (wl - cl) * m2;
        R[i] = cr + (wr - cr) * m2;
    }

    for (int b = 0; b < maxBands; ++b) bandLevel[b].store (b < nb ? env[b] : 0.0f, std::memory_order_relaxed);
}
