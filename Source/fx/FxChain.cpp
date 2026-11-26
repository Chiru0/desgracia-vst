#include "FxChain.h"

namespace
{
    inline double divBeats (int i)
    {
        static const double b[] = { 4.0, 2.0, 1.0, 2.0 / 3.0, 0.5, 1.0 / 3.0, 0.75, 0.25, 1.0 / 6.0, 0.125 };
        return b[juce::jlimit (0, 9, i)];
    }

    inline float softCeiling (float x)
    {
        const float a = std::abs (x);
        if (a < 0.85f) return x;
        const float y = 0.85f + 0.15f * std::tanh ((a - 0.85f) / 0.15f);
        return x < 0 ? -y : y;
    }
}

void FxChain::prepare (double sampleRate, int maxBlock)
{
    sr = sampleRate;
    maxBlockSize = juce::jmax (16, maxBlock);

    oversampler = std::make_unique<juce::dsp::Oversampling<float>> (2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
    oversampler->initProcessing ((size_t) maxBlockSize);

    for (auto* f : { &drvFade, &crsFade, &eqFade, &choFade, &dlyFade, &revFade }) f->prepare (sr, 25.0f);

    drvGain.reset (sr * 2.0, 0.03); drvMixS.reset (sr * 2.0, 0.03);
    crsStepsS.reset (sr, 0.03);
    widthS.reset (sr, 0.03); masterS.reset (sr, 0.03);

    juce::dsp::ProcessSpec spec { sr, (juce::uint32) maxBlockSize, 2 };
    chorus.prepare (spec);
    chorus.setCentreDelay (7.0f);
    chorus.setFeedback (0.0f);
    chorus.setMix (1.0f);
    choBuf.setSize (2, maxBlockSize);

    dlyLine.setSize (2, (int) (sr * 2.6) + 8);
    revBuf.setSize (2, maxBlockSize);
    reverb.prepare (spec);

    reset();
}

void FxChain::reset()
{
    if (oversampler) oversampler->reset();
    chorus.reset();
    reverb.reset();
    dlyLine.clear();
    dlyWrite = 0; dlySmoothed = 0.0f;
    dlyLp[0] = dlyLp[1] = dlyHp[0] = dlyHp[1] = 0.0f;
    crsAcc = 0.0f; crsHeld[0] = crsHeld[1] = 0.0f;
    eqL.reset(); eqM.reset(); eqH.reset();
    dlyTail = revTail = 0;
}

void FxChain::process (juce::AudioBuffer<float>& buffer, const FxParams& p, double bpm)
{
    const int n = buffer.getNumSamples();
    if (n == 0) return;
    jassert (n <= maxBlockSize);
    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);

    // ---------------- DRIVE (always runs through the oversampler for constant phase)
    {
        drvGain.setTargetValue (juce::Decibels::decibelsToGain (p.drvAmt * 36.0f));
        drvMixS.setTargetValue (p.drvMix);
        juce::dsp::AudioBlock<float> block (buffer);
        auto up = oversampler->processSamplesUp (block);
        float* uL = up.getChannelPointer (0);
        float* uR = up.getChannelPointer (1);
        const int un = (int) up.getNumSamples();
        const int type = p.drvType;
        for (int i = 0; i < un; ++i)
        {
            const float fade = drvFade.next (p.drvOn);
            const float g = drvGain.getNextValue();
            const float mix = drvMixS.getNextValue() * fade;
            if (mix <= 0.0f) continue;
            const float comp = 1.0f / std::sqrt (g);
            for (float* s : { uL + i, uR + i })
            {
                const float x = *s, gx = x * g;
                float y;
                switch (type)
                {
                    case 1:  y = juce::jlimit (-1.0f, 1.0f, gx * 0.9f); break;
                    case 2:  y = dsp::foldTri (gx * 0.8f) * 0.9f; break;
                    case 3:  y = (std::tanh (gx + 0.4f) - 0.3799f) * 1.25f; break;
                    default: y = std::tanh (gx); break;
                }
                *s = x + (y * comp * 1.3f - x) * mix;
            }
        }
        oversampler->processSamplesDown (block);

        // asymmetric shapers (TUBE / HARD on uneven waves) create DC: block it
        const float R_ = 1.0f - dsp::twoPi * 8.0f / (float) sr;
        for (int i = 0; i < n; ++i)
        {
            const float l = L[i], r = R[i];
            const float yl = l - dcX[0] + R_ * dcY[0]; dcX[0] = l; dcY[0] = yl; L[i] = yl;
            const float yr = r - dcX[1] + R_ * dcY[1]; dcX[1] = r; dcY[1] = yr; R[i] = yr;
        }
    }

    // ---------------- CRUSH
    if (! crsFade.silent (p.crsOn))
    {
        crsStepsS.setTargetValue (std::exp2 (p.crsBits - 1.0f));
        const float inc = p.crsRate / (float) sr;
        for (int i = 0; i < n; ++i)
        {
            const float fade = crsFade.next (p.crsOn) * p.crsMix;
            const float steps = crsStepsS.getNextValue();
            crsAcc += inc;
            if (crsAcc >= 1.0f)
            {
                crsAcc -= std::floor (crsAcc);
                crsHeld[0] = std::round (L[i] * steps) / steps;
                crsHeld[1] = std::round (R[i] * steps) / steps;
            }
            L[i] += (crsHeld[0] - L[i]) * fade;
            R[i] += (crsHeld[1] - R[i]) * fade;
        }
    }

    // ---------------- EQ
    if (! eqFade.silent (p.eqOn))
    {
        if (std::abs (eqCur[0] - p.eqLow) > 0.01f)  { eqCur[0] = p.eqLow;  eqL.lowShelf (sr, 140.0, p.eqLow); }
        if (std::abs (eqCur[1] - p.eqMid) > 0.01f || std::abs (eqCur[2] - p.eqMidFreq) > 1.0f)
        { eqCur[1] = p.eqMid; eqCur[2] = p.eqMidFreq; eqM.peak (sr, p.eqMidFreq, 0.9, p.eqMid); }
        if (std::abs (eqCur[3] - p.eqHigh) > 0.01f) { eqCur[3] = p.eqHigh; eqH.highShelf (sr, 5500.0, p.eqHigh); }
        for (int i = 0; i < n; ++i)
        {
            const float fade = eqFade.next (p.eqOn);
            float l = eqH.tick (eqM.tick (eqL.tick (L[i], 0), 0), 0);
            float r = eqH.tick (eqM.tick (eqL.tick (R[i], 1), 1), 1);
            L[i] += (l - L[i]) * fade;
            R[i] += (r - R[i]) * fade;
        }
    }

    // ---------------- CHORUS
    if (! choFade.silent (p.choOn))
    {
        chorus.setRate (p.choRate);
        chorus.setDepth (p.choDepth);
        for (int c = 0; c < 2; ++c) choBuf.copyFrom (c, 0, buffer, c, 0, n);
        juce::dsp::AudioBlock<float> cb (choBuf.getArrayOfWritePointers(), 2, (size_t) n);
        chorus.process (juce::dsp::ProcessContextReplacing<float> (cb));
        const float* wl = choBuf.getReadPointer (0);
        const float* wr = choBuf.getReadPointer (1);
        for (int i = 0; i < n; ++i)
        {
            const float m = choFade.next (p.choOn) * p.choMix * 0.5f;
            L[i] += (wl[i] - L[i]) * m;
            R[i] += (wr[i] - R[i]) * m;
        }
    }

    // ---------------- DELAY (send style, tail keeps ringing after switch-off)
    if (p.dlyOn || dlyFade.v > 0.0f || dlyTail > 0)
    {
        const int len = dlyLine.getNumSamples();
        float* dl = dlyLine.getWritePointer (0);
        float* dr = dlyLine.getWritePointer (1);
        double secs = p.dlySync ? divBeats (p.dlyDiv) * 60.0 / juce::jmax (30.0, bpm) : p.dlyTime * 0.001;
        const float target = (float) juce::jlimit (16.0, (double) len - 4.0, juce::jmin (2.5, secs) * sr);
        if (dlySmoothed <= 0.0f) dlySmoothed = target;
        const float lpA = 1.0f - std::exp (-dsp::twoPi * p.dlyTone / (float) sr);
        const float hpA = 1.0f - std::exp (-dsp::twoPi * 140.0f / (float) sr);
        float peak = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            const float send = dlyFade.next (p.dlyOn);
            dlySmoothed += (target - dlySmoothed) * 0.0006f;
            float rp = (float) dlyWrite - dlySmoothed;
            if (rp < 0) rp += (float) len;
            const int i0 = (int) rp; const float t = rp - (float) i0;
            const int i1 = (i0 + 1) % len;
            const float rl = dl[i0] + (dl[i1] - dl[i0]) * t;
            const float rr = dr[i0] + (dr[i1] - dr[i0]) * t;

            dlyLp[0] += lpA * (rl - dlyLp[0]); dlyLp[1] += lpA * (rr - dlyLp[1]);
            dlyHp[0] += hpA * (dlyLp[0] - dlyHp[0]); dlyHp[1] += hpA * (dlyLp[1] - dlyHp[1]);
            const float fl = dlyLp[0] - dlyHp[0], fr = dlyLp[1] - dlyHp[1];

            float wl, wr;
            if (p.dlyPing) { wl = 0.5f * (L[i] + R[i]) * send + p.dlyFb * fr; wr = p.dlyFb * fl; }
            else           { wl = L[i] * send + p.dlyFb * fl;              wr = R[i] * send + p.dlyFb * fr; }
            wl = std::tanh (wl); wr = std::tanh (wr);
            dl[dlyWrite] = wl; dr[dlyWrite] = wr;
            if (++dlyWrite >= len) dlyWrite = 0;
            peak = std::max (peak, std::abs (wl) + std::abs (wr));

            L[i] += rl * p.dlyMix;
            R[i] += rr * p.dlyMix;
        }
        dlyTail = peak > 1.0e-6f ? (int) (sr * 3.0) : juce::jmax (0, dlyTail - n);
        if (! p.dlyOn && dlyFade.v <= 0.0f && peak <= 1.0e-6f) dlyTail = 0;
    }

    // ---------------- REVERB (send style)
    if (p.revOn || revFade.v > 0.0f || revTail > 0)
    {
        revParams.roomSize = 0.3f + 0.68f * p.revSize;
        revParams.damping = p.revDamp;
        revParams.wetLevel = 1.0f;
        revParams.dryLevel = 0.0f;
        revParams.width = 1.0f;
        revParams.freezeMode = 0.0f;
        reverb.setParameters (revParams);
        float* bl = revBuf.getWritePointer (0);
        float* br = revBuf.getWritePointer (1);
        for (int i = 0; i < n; ++i)
        {
            const float send = revFade.next (p.revOn);
            bl[i] = L[i] * send; br[i] = R[i] * send;
        }
        juce::dsp::AudioBlock<float> rb (revBuf.getArrayOfWritePointers(), 2, (size_t) n);
        reverb.process (juce::dsp::ProcessContextReplacing<float> (rb));
        float peak = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            L[i] += bl[i] * p.revMix * 0.6f;
            R[i] += br[i] * p.revMix * 0.6f;
            peak = std::max (peak, std::abs (bl[i]) + std::abs (br[i]));
        }
        if (p.revOn || revFade.v > 0.0f || peak > 1.0e-6f) revTail = (int) (sr * 2.0);
        else revTail = juce::jmax (0, revTail - n);
    }

    // ---------------- WIDTH + MASTER
    widthS.setTargetValue (p.width);
    masterS.setTargetValue (juce::Decibels::decibelsToGain (p.masterDb));
    for (int i = 0; i < n; ++i)
    {
        const float w = widthS.getNextValue();
        const float g = masterS.getNextValue();
        const float m = 0.5f * (L[i] + R[i]);
        const float s = 0.5f * (L[i] - R[i]) * w;
        L[i] = softCeiling ((m + s) * g);
        R[i] = softCeiling ((m - s) * g);
    }
}
