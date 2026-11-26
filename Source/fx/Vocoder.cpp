#include "Vocoder.h"

namespace
{
    // A E I O U formants (F1 F2 F3) and gains
    const float vf[5][3] = { { 800, 1150, 2900 }, { 420, 1800, 2600 }, { 290, 2300, 3100 }, { 480, 820, 2800 }, { 330, 700, 2500 } };
    const float vg[5][3] = { { 1.0f, 0.6f, 0.25f }, { 1.0f, 0.55f, 0.3f }, { 1.0f, 0.5f, 0.35f }, { 1.0f, 0.7f, 0.15f }, { 1.0f, 0.45f, 0.12f } };

    // sweep = per step the vowel glides from vowel[0] to vowel[1] (wa-wa, yoi-yoi)
    struct Pattern { int len; int vowel[5]; int cons[5]; bool continuous; bool sweep; };
    const Pattern patterns[8] = {
        { 1, { 4, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 }, false, true  },   // WA-WA-WA  (U -> A each step)
        { 4, { 1, 0, 2, 0, 0 }, { 1, 1, 1, 0, 0 }, false, false },   // DES-GRA-CI-A
        { 5, { 0, 1, 2, 3, 4 }, { 1, 1, 1, 1, 1 }, false, false },   // A-E-I-O-U
        { 4, { 4, 0, 4, 0, 0 }, { 1, 0, 1, 0, 0 }, false, false },   // U-A U-A
        { 1, { 2, 3, 0, 0, 0 }, { 0, 0, 0, 0, 0 }, false, true  },   // YOI-YOI   (I -> O each step)
        { 1, { 0, 0, 0, 0, 0 }, { 1, 0, 0, 0, 0 }, false, false },   // HABLA RANDOM
        { 1, { 0, 0, 0, 0, 0 }, { 1, 0, 0, 0, 0 }, false, false },   // RUIDOS RANDOM
        { 1, { 4, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 }, true,  false } }; // WAH LENTO (morph)

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
    envIn = envOut = 0.0f; autoGain = 1.0f; buzzHz = buzzTarget = 132.0f; breath = 0.12f;
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
    const int pi = juce::jlimit (0, 7, p.pattern);
    const auto& pat = patterns[pi];

    if (newStep)
    {
        newStep = false;
        if (pi == 5) { vowelTarget = (float) (rng.nextU() % 5); curCons = (rng.nextU() & 1) ? 1 : 0; buzzTarget = 132.0f; breath = 0.12f; }
        else if (pi == 6)
        {
            // robot noises: random vowel, random pitch jumps, random breath / clicks
            vowelTarget = (float) (rng.nextU() % 5);
            curCons = (rng.nextU() % 3) != 0 ? 1 : 0;
            buzzTarget = 70.0f * std::exp2 ((float) (rng.nextU() % 1000) / 1000.0f * 2.6f);
            breath = 0.05f + (float) (rng.nextU() % 100) / 100.0f * 0.6f;
        }
        else if (pat.sweep) { vowelPos = (float) pat.vowel[0]; vowelTarget = (float) pat.vowel[1]; curCons = 0; buzzTarget = 132.0f; breath = 0.12f; }
        else if (! pat.continuous) { const int s = (int) (stepIndex % pat.len); vowelTarget = (float) pat.vowel[s]; curCons = pat.cons[s]; buzzTarget = 132.0f; breath = 0.12f; }
        else { vowelTarget = (float) pat.vowel[0]; buzzTarget = 132.0f; breath = 0.12f; }
        if (! pat.continuous && curCons) burst = 1.0f;
        currentVowel = (int) vowelTarget;
    }

    const float t = (float) stepPhase;   // 0..1 within the syllable
    if (pi == 7)                         // WAH: U -> A -> U across two steps
    {
        const float w = 0.5f - 0.5f * std::cos (dsp::twoPi * ((float) (stepIndex % 2) + t) * 0.5f);
        vowelPos += ((4.0f * (1.0f - w)) - vowelPos) * 0.002f;
        currentVowel = vowelPos > 2.0f ? 4 : 0;
    }
    else if (pat.sweep)
    {
        // fast glide in the first 35% of the step: "wa", "yoi"
        const float k = juce::jlimit (0.0f, 1.0f, t / 0.35f);
        const float shaped = k * k * (3.0f - 2.0f * k);
        vowelPos = (float) pat.vowel[0] + ((float) pat.vowel[1] - (float) pat.vowel[0]) * shaped;
        currentVowel = shaped > 0.5f ? pat.vowel[1] : pat.vowel[0];
    }
    else
        vowelPos += (vowelTarget - vowelPos) * (pi == 6 ? 0.006f : 0.0025f);

    // syllable gate: on, hold, short gap at the end of each step
    const float gTarget = pat.continuous ? 1.0f : (t < 0.78f ? 1.0f : 0.0f);
    gate += (gTarget - gate) * (gTarget > gate ? 0.004f : 0.0015f);
    voiceGate = gate;

    // glottal buzz + breath
    buzzHz += (buzzTarget - buzzHz) * 0.003f;
    buzzPhase += buzzHz / (float) sr;
    if (buzzPhase >= 1.0f) buzzPhase -= 1.0f;
    const float src = (2.0f * buzzPhase - 1.0f) * 0.8f + rng.bipolar() * breath;

    // formants
    const float vp = juce::jlimit (0.0f, 4.0f, vowelPos);
    const int i0 = juce::jlimit (0, 3, (int) vp);
    const float fr = juce::jlimit (0.0f, 1.0f, vp - (float) i0);
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

void Vocoder::process (float* L, float* R, int n, const VocoderParams& p, double bpm, bool playing, double ppq)
{
    if (! p.on && fade <= 0.0f) return;

    const int nb = juce::jlimit (6, maxBands, p.bands);
    if (nb != cfgBands || std::abs (p.shift - cfgShift) > 0.01f || std::abs (p.q - cfgQ) > 0.001f)
        configure (nb, p.shift, p.q);

    const float atk = 1.0f - std::exp (-1.0f / ((0.0015f + (1.0f - p.speed) * 0.012f) * (float) sr));
    const float rel = 1.0f - std::exp (-1.0f / ((0.012f + (1.0f - p.speed) * 0.11f) * (float) sr));
    const double beats = divBeats (p.rateDiv);
    stepLenSamples = beats * 60.0 / juce::jmax (30.0, bpm) * sr;
    // auto level: the vocoded signal follows the loudness of the dry synth, so it never goes silent
    // just because the patch has little energy in some bands (e.g. a sine).
    const float lvA = 1.0f - std::exp (-1.0f / (0.25f * (float) sr));
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

        const float m = voiceSample (p);

        const float cl = L[i], cr = R[i];
        // enrich the carrier with harmonics so every band has something to filter
        const float el = cl + 0.35f * std::tanh (cl * 6.0f), er = cr + 0.35f * std::tanh (cr * 6.0f);
        float ol = 0.0f, orr = 0.0f;
        for (int b = 0; b < nb; ++b)
        {
            float lp, bp, hp;
            mod[b].tick (m, lp, bp, hp);
            const float a = std::abs (bp * mod[b].k);
            env[b] += (a - env[b]) * (a > env[b] ? atk : rel);
            const float ex = env[b] * std::sqrt (env[b]);   // expand: stronger vowel contrast
            carL[b].tick (el, lp, bp, hp); ol += bp * carL[b].k * ex;
            carR[b].tick (er, lp, bp, hp); orr += bp * carR[b].k * ex;
        }
        // sibilance: let the modulator's top end through as noise
        const float hf = env[nb - 1] + env[nb - 2];
        sibLp += sibA * (rng.bipolar() - sibLp);
        const float s = (rng.bipolar() - sibLp) * hf * p.sib * 1.5f;

        envIn  += lvA * (std::abs (cl) + std::abs (cr) - envIn);
        envOut += lvA * (std::abs (ol) + std::abs (orr) - envOut);
        if (envIn > 1.0e-4f)
        {
            const float target = juce::jlimit (1.0f, 400.0f, 0.9f * envIn / juce::jmax (1.0e-6f, envOut));
            autoGain += (target - autoGain) * lvA;
        }
        const float wl = juce::jlimit (-2.0f, 2.0f, ol * autoGain + s * envIn * 4.0f);
        const float wr = juce::jlimit (-2.0f, 2.0f, orr * autoGain + s * envIn * 4.0f);
        const float m2 = p.mix * fade;
        L[i] = cl + (wl - cl) * m2;
        R[i] = cr + (wr - cr) * m2;
    }

    for (int b = 0; b < maxBands; ++b) bandLevel[b].store (b < nb ? env[b] : 0.0f, std::memory_order_relaxed);
}
