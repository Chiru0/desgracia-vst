#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>

// Small, allocation-free DSP building blocks used by the voices.
namespace dsp
{
    constexpr float twoPi = 6.283185307179586f;
    constexpr float pi    = 3.141592653589793f;

    inline float clampf (float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }
    inline float frac (float x) { return x - std::floor (x); }

    // PolyBLEP residual for band-limiting discontinuities
    inline float polyBlep (float t, float dt)
    {
        if (t < dt)        { t /= dt;           return t + t - t * t - 1.0f; }
        if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
        return 0.0f;
    }

    // triangle wavefolder: maps any x back into [-1, 1]
    inline float foldTri (float x)
    {
        float y = frac ((x + 1.0f) * 0.25f);
        return 1.0f - 4.0f * std::abs (y - 0.5f) ;
    }

    // fast sine for phase in [0,1)
    inline float sin01 (float p) { return std::sin (twoPi * p); }

    struct Rng
    {
        uint32_t s = 0x12345678u;
        void seed (uint32_t v) { s = v ? v : 0x9e3779b9u; }
        inline uint32_t nextU() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
        inline float bipolar() { return (float) (int32_t) nextU() * (1.0f / 2147483648.0f); }
        inline float unipolar() { return (float) (nextU() >> 8) * (1.0f / 16777216.0f); }
    };

    enum Wave { Sine = 0, Tri, Saw, Pulse, Toy, Formant, Buzz, Metal };

    // One oscillator. "shape" changes the character per wave (see choices::shapeName).
    struct Osc
    {
        float p = 0.0f;   // main phase [0,1)
        float q = 0.0f;   // secondary phase (METAL modulator)

        void reset (float phase, float phase2) { p = phase; q = phase2; }

        inline float tick (int wave, float dt, float shape, float pm)
        {
            float ph = pm != 0.0f ? frac (p + pm) : p;
            float y = 0.0f;

            switch (wave)
            {
                case Sine:
                    y = shape > 0.001f ? std::sin (twoPi * ph + shape * 2.2f * std::sin (twoPi * ph)) : sin01 (ph);
                    break;
                case Tri:
                {
                    float t = 1.0f - 4.0f * std::abs (ph - 0.5f);
                    y = shape > 0.001f ? foldTri (t * (1.0f + 3.0f * shape)) : t;
                    break;
                }
                case Saw:
                {
                    float s = 2.0f * ph - 1.0f - polyBlep (ph, dt);
                    if (shape > 0.001f)
                    {
                        float p2 = frac (2.0f * ph);
                        float s2 = 2.0f * p2 - 1.0f - polyBlep (p2, std::min (0.5f, 2.0f * dt));
                        s = (s + shape * s2) / (1.0f + 0.5f * shape);
                    }
                    y = s;
                    break;
                }
                case Pulse:
                case Buzz:
                {
                    float pw = wave == Pulse ? 0.5f - 0.45f * shape : 0.22f - 0.2f * shape;
                    float v = ph < pw ? 1.0f : -1.0f;
                    v += polyBlep (ph, dt);
                    v -= polyBlep (frac (ph - pw + 1.0f), dt);
                    v -= (2.0f * pw - 1.0f);              // remove DC
                    y = wave == Pulse ? v : v * 0.7f;
                    break;
                }
                case Toy:
                {
                    float steps = 2.0f + (1.0f - shape) * 13.0f;
                    float s = sin01 (ph) * 0.85f + 0.3f * sin01 (frac (2.0f * ph));
                    y = std::round (s * steps) / steps;
                    break;
                }
                case Formant:
                {
                    float r = 1.0f + shape * 7.0f;
                    float w = 1.0f - ph;
                    y = 1.6f * w * std::sin (twoPi * ph * r);
                    break;
                }
                case Metal:
                {
                    float idx = 0.6f + shape * 3.4f;
                    y = std::sin (twoPi * ph + idx * sin01 (q));
                    break;
                }
                default: break;
            }

            p += dt; if (p >= 1.0f) p -= 1.0f;
            if (wave == Metal) { q += dt * 2.76f; if (q >= 1.0f) q -= std::floor (q); }
            return y;
        }
    };

    // Zavalishin TPT state variable filter
    struct SVF
    {
        float ic1 = 0.0f, ic2 = 0.0f;
        float a1 = 1.0f, a2 = 0.0f, a3 = 0.0f, k = 2.0f;

        void reset() { ic1 = ic2 = 0.0f; }

        void set (float g, float kk)
        {
            k = kk;
            a1 = 1.0f / (1.0f + g * (g + k));
            a2 = g * a1;
            a3 = g * a2;
        }

        inline void tick (float v0, float& lp, float& bp, float& hp)
        {
            float v3 = v0 - ic2;
            float v1 = a1 * ic1 + a2 * v3;
            float v2 = ic2 + a2 * ic1 + a3 * v3;
            ic1 = 2.0f * v1 - ic1;
            ic2 = 2.0f * v2 - ic2;
            lp = v2; bp = v1; hp = v0 - k * v1 - v2;
        }
    };

    // ADSR with linear attack and exponential decay/release.
    // Re-triggering starts the attack from the current level (no clicks).
    struct Env
    {
        enum Stage { Idle, Attack, Decay, Sustain, Release };
        Stage stage = Idle;
        float level = 0.0f;
        float attackInc = 1.0f, decayCoef = 0.0f, releaseCoef = 0.0f, sustain = 1.0f;
        float sr = 44100.0f;

        void setSampleRate (float s) { sr = s; }

        void setParams (float a, float d, float s, float r)
        {
            attackInc = 1.0f / std::max (1.0f, a * sr);
            decayCoef = std::exp (-4.6f / std::max (1.0f, d * sr));     // ~ -40dB over 'd'
            releaseCoef = std::exp (-4.6f / std::max (1.0f, r * sr));
            sustain = s;
        }

        void noteOn()  { stage = Attack; }
        void noteOff() { if (stage != Idle) stage = Release; }
        void kill()    { stage = Idle; level = 0.0f; }
        bool active() const { return stage != Idle; }

        inline float tick()
        {
            switch (stage)
            {
                case Attack:
                    level += attackInc;
                    if (level >= 1.0f) { level = 1.0f; stage = Decay; }
                    break;
                case Decay:
                    level = sustain + (level - sustain) * decayCoef;
                    if (level - sustain < 1.0e-4f) { level = sustain; stage = Sustain; }
                    break;
                case Sustain:
                    // follow sustain changes smoothly
                    level += (sustain - level) * 0.001f;
                    if (sustain <= 0.0f && level < 1.0e-4f) { level = 0.0f; stage = Idle; }
                    break;
                case Release:
                    level *= releaseCoef;
                    if (level < 1.0e-4f) { level = 0.0f; stage = Idle; }
                    break;
                case Idle: default: break;
            }
            return level;
        }
    };

    // RBJ biquad for the EQ
    struct Biquad
    {
        float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        float z1[2] = { 0, 0 }, z2[2] = { 0, 0 };

        void reset() { z1[0] = z1[1] = z2[0] = z2[1] = 0; }

        inline float tick (float x, int ch)
        {
            float y = b0 * x + z1[ch];
            z1[ch] = b1 * x - a1 * y + z2[ch];
            z2[ch] = b2 * x - a2 * y;
            return y;
        }

        void setCoefs (double B0, double B1, double B2, double A0, double A1, double A2)
        {
            b0 = (float) (B0 / A0); b1 = (float) (B1 / A0); b2 = (float) (B2 / A0);
            a1 = (float) (A1 / A0); a2 = (float) (A2 / A0);
        }

        void lowShelf (double sr, double f, double gainDb)
        {
            double A = std::pow (10.0, gainDb / 40.0), w = 2.0 * 3.141592653589793 * f / sr;
            double c = std::cos (w), s = std::sin (w), al = s / 2.0 * std::sqrt (2.0), sa = 2.0 * std::sqrt (A) * al;
            setCoefs (A * ((A + 1) - (A - 1) * c + sa), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - sa),
                      (A + 1) + (A - 1) * c + sa, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - sa);
        }

        void highShelf (double sr, double f, double gainDb)
        {
            double A = std::pow (10.0, gainDb / 40.0), w = 2.0 * 3.141592653589793 * f / sr;
            double c = std::cos (w), s = std::sin (w), al = s / 2.0 * std::sqrt (2.0), sa = 2.0 * std::sqrt (A) * al;
            setCoefs (A * ((A + 1) + (A - 1) * c + sa), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - sa),
                      (A + 1) - (A - 1) * c + sa, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - sa);
        }

        void peak (double sr, double f, double q, double gainDb)
        {
            double A = std::pow (10.0, gainDb / 40.0), w = 2.0 * 3.141592653589793 * f / sr;
            double c = std::cos (w), al = std::sin (w) / (2.0 * q);
            setCoefs (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
        }
    };
}
