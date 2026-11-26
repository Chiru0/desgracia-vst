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
    reset();
}

void Vocoder::reset()
{
    for (int k = 0; k < 3; ++k) { fL[k].reset(); fR[k].reset(); }
    consL.reset(); consR.reset();
    for (auto& b : bandLevel) b = 0.0f;
    gate = burst = 0.0f; jump = jumpTarget = 0.0f;
    envIn = envOut = 0.0f; autoGain = 1.0f;
    stepPhase = 0.0; stepIndex = 0; newStep = true; coefCounter = 0;
}

void Vocoder::nextSyllable (const VocoderParams& p)
{
    const int pi = juce::jlimit (0, 7, p.pattern);
    const auto& pat = patterns[pi];
    jumpTarget = 0.0f;
    if (pi == 5) { vowelTarget = (float) (rng.nextU() % 5); curCons = (rng.nextU() & 1) ? 1 : 0; }
    else if (pi == 6)
    {
        // robot noises: random vowel, random formant jumps, random clicks
        vowelTarget = (float) (rng.nextU() % 5);
        curCons = (rng.nextU() % 3) != 0 ? 1 : 0;
        jumpTarget = rng.bipolar() * 9.0f;
    }
    else if (pat.sweep) { vowelPos = (float) pat.vowel[0]; vowelTarget = (float) pat.vowel[1]; curCons = 0; }
    else if (! pat.continuous) { const int s = (int) (stepIndex % pat.len); vowelTarget = (float) pat.vowel[s]; curCons = pat.cons[s]; }
    else vowelTarget = (float) pat.vowel[0];
    if (! pat.continuous && curCons) burst = 1.0f;
    currentVowel = (int) vowelTarget;
}

void Vocoder::updateFormants (const VocoderParams& p)
{
    const float vp = juce::jlimit (0.0f, 4.0f, vowelPos);
    const int i0 = juce::jlimit (0, 3, (int) vp);
    const float fr = vp - (float) i0;
    const float shift = std::exp2 ((p.shift + jump) / 12.0f);
    // more "bands" = sharper formants = clearer words; METAL = extra resonance
    const float Q = 2.5f + (float) (juce::jlimit (6, 24, p.bands) - 6) / 18.0f * 7.0f + p.q * 6.0f;
    const float nyq = 0.45f * (float) sr;
    for (int k = 0; k < 3; ++k)
    {
        fFreq[k] = juce::jmin (nyq, (vf[i0][k] + (vf[i0 + 1][k] - vf[i0][k]) * fr) * shift);
        fGain[k] = vg[i0][k] + (vg[i0 + 1][k] - vg[i0][k]) * fr;
        const float g = std::tan (dsp::pi * fFreq[k] / (float) sr);
        const float kk = 1.0f / (Q * (k == 0 ? 0.8f : 1.0f));
        fL[k].set (g, kk); fR[k].set (g, kk);
    }
}

void Vocoder::process (float* L, float* R, int n, const VocoderParams& p, double bpm, bool playing, double ppq)
{
    if (! p.on && fade <= 0.0f) return;

    const double beats = divBeats (p.rateDiv);
    stepLenSamples = beats * 60.0 / juce::jmax (30.0, bpm) * sr;
    const int pi = juce::jlimit (0, 7, p.pattern);
    const auto& pat = patterns[pi];
    const float glide = 0.0006f + p.speed * 0.006f;
    const float lvA = 1.0f - std::exp (-1.0f / (0.2f * (float) sr));
    const float gAtk = 1.0f - std::exp (-1.0f / (0.004f * (float) sr));
    const float gRel = 1.0f - std::exp (-1.0f / (0.012f * (float) sr));
    consL.set (std::tan (dsp::pi * 3500.0f / (float) sr), 1.2f);
    consR.set (std::tan (dsp::pi * 3500.0f / (float) sr), 1.2f);

    for (int i = 0; i < n; ++i)
    {
        fade = p.on ? juce::jmin (1.0f, fade + fadeStep) : juce::jmax (0.0f, fade - fadeStep);

        // step clock (locked to the host grid when playing)
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
        if (newStep) { newStep = false; nextSyllable (p); }

        const float t = (float) stepPhase;
        if (pi == 7)        // WAH: U -> A -> U across two steps
        {
            const float w = 0.5f - 0.5f * std::cos (dsp::twoPi * ((float) (stepIndex % 2) + t) * 0.5f);
            vowelPos = 4.0f * (1.0f - w);
            currentVowel = vowelPos > 2.0f ? 4 : 0;
        }
        else if (pat.sweep) // "wa", "yoi": quick glide at the start of each step
        {
            const float k = juce::jlimit (0.0f, 1.0f, t / 0.35f);
            const float s = k * k * (3.0f - 2.0f * k);
            vowelPos = (float) pat.vowel[0] + ((float) pat.vowel[1] - (float) pat.vowel[0]) * s;
            currentVowel = s > 0.5f ? pat.vowel[1] : pat.vowel[0];
        }
        else
            vowelPos += (vowelTarget - vowelPos) * glide;
        jump += (jumpTarget - jump) * 0.004f;

        if (--coefCounter <= 0) { coefCounter = 16; updateFormants (p); }

        // syllable gate: on, then a short gap at the end of each step
        const float gTarget = pat.continuous ? 1.0f : (t < 0.78f ? 1.0f : 0.0f);
        gate += (gTarget - gate) * (gTarget > gate ? gAtk : gRel);

        const float cl = L[i], cr = R[i];
        // gentle saturation so even a sine has harmonics for the formants to shape
        const float el = cl + 0.3f * std::tanh (cl * 4.0f), er = cr + 0.3f * std::tanh (cr * 4.0f);
        float ol = 0.0f, orr = 0.0f;
        for (int k = 0; k < 3; ++k)
        {
            float lp, bp, hp;
            fL[k].tick (el, lp, bp, hp); ol  += bp * fL[k].k * fGain[k];
            fR[k].tick (er, lp, bp, hp); orr += bp * fR[k].k * fGain[k];
        }
        // consonant: a short bright click made from the synth itself (no noise)
        if (burst > 0.0005f)
        {
            float lp, bp, hp;
            consL.tick (el, lp, bp, hp); ol += hp * burst * p.sib * 1.5f;
            consR.tick (er, lp, bp, hp); orr += hp * burst * p.sib * 1.5f;
            burst *= 1.0f - 1.0f / (0.015f * (float) sr);
        }

        // auto level: match the dry loudness; only adapts while a syllable is sounding
        envIn  += lvA * (std::abs (cl) + std::abs (cr) - envIn);
        envOut += lvA * (std::abs (ol) + std::abs (orr) - envOut);
        if (envIn > 1.0e-4f && gate > 0.5f)
        {
            const float target = juce::jlimit (1.0f, 12.0f, envIn / juce::jmax (1.0e-6f, envOut));
            autoGain += (target - autoGain) * lvA;
        }
        const float wl = juce::jlimit (-2.0f, 2.0f, ol * autoGain * gate);
        const float wr = juce::jlimit (-2.0f, 2.0f, orr * autoGain * gate);
        const float m2 = p.mix * fade;
        L[i] = cl + (wl - cl) * m2;
        R[i] = cr + (wr - cr) * m2;
    }

    voiceGate = gate;
    // visualiser: show where the formants are right now
    const int nb = juce::jlimit (6, maxBands, p.bands);
    const float ratio = std::pow (7500.0f / 110.0f, 1.0f / (float) (nb - 1));
    for (int b = 0; b < maxBands; ++b)
    {
        float v = 0.0f;
        if (b < nb)
        {
            const float fb = 110.0f * std::pow (ratio, (float) b);
            for (int k = 0; k < 3; ++k)
            {
                const float d = std::log2 (fb / fFreq[k]);
                v += fGain[k] * std::exp (-d * d / 0.08f);
            }
            v *= gate * 0.35f * juce::jmin (1.0f, envIn * 8.0f);
        }
        bandLevel[b].store (v, std::memory_order_relaxed);
    }
}
