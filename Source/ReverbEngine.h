#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include <vector>

// DreamyReverb DSP engine.
// A damped feedback-delay reverb with:
//  - RT60-accurate feedback (Decay is in seconds)
//  - Size controls the comb delay time (40..300 ms)
//  - Damping controls a one-pole low-pass inside the feedback loop
//  - Shimmer: granular octave/fractional pitch-shift inside the feedback loop
//  - Depth/Rate: a slow LFO that modulates the feedback gain (tape-style drift)
//  - Optional HPF / LPF on the wet signal
//
// The engine keeps ALL state in members (no block-boundary resets), so the
// output is independent of the host's block size.

class ReverbEngine
{
public:
    struct Parameters
    {
        float decay    = 4.2f;    // RT60 in seconds (0.1 .. 10)
        float size     = 85.0f;   // 0 .. 100  -> comb delay 40..300 ms
        float damping  = 4500.0f; // Hz (100 .. 20000)
        float shimmer  = 6.0f;    // semitones (0 .. 12) shift amount in the loop
        float mix      = 40.0f;   // dry/wet percent (0 .. 100)
        float depth    = 25.0f;   // LFO depth percent (0 .. 100)
        float rate     = 0.5f;    // LFO rate Hz (0.05 .. 8)
        bool  hpf      = false;   // 120 Hz one-pole high-pass on the wet signal
        bool  lpf      = false;   // 8 kHz one-pole low-pass on the wet signal
    };

    void prepare (double sampleRate, int /*maxBlockSize*/, int numChannels)
    {
        sr = (sampleRate > 0.0 ? sampleRate : 48000.0);

        const auto lineLen = (size_t) std::ceil (sr * 1.0) + 8;   // 1 s comb line
        const auto shimLen = (size_t) std::ceil (sr * 0.35) + 8;  // 0.35 s shifter history

        chans.resize ((size_t) juce::jmax (1, numChannels));

        for (auto& c : chans)
        {
            c.line.assign (lineLen, 0.0f);
            c.shim.assign (shimLen, 0.0f);
            c.w = 0;
            c.sw = 0;
            c.damp = 0.0f;
            c.hpState = 0.0f;
            c.lpState = 0.0f;
            c.grain = 0.0f;
        }

        lfoPhase = 0.0f;
    }

    void reset()
    {
        for (auto& c : chans)
        {
            std::fill (c.line.begin(), c.line.end(), 0.0f);
            std::fill (c.shim.begin(), c.shim.end(), 0.0f);
            c.w = 0;
            c.sw = 0;
            c.damp = 0.0f;
            c.hpState = 0.0f;
            c.lpState = 0.0f;
            c.grain = 0.0f;
        }

        lfoPhase = 0.0f;
    }

    void process (juce::AudioBuffer<float>& buffer, const Parameters& p)
    {
        const int n = buffer.getNumSamples();
        const int numCh = juce::jmin (buffer.getNumChannels(), (int) chans.size());

        if (n <= 0 || numCh <= 0 || chans.empty())
            return;

        const float twoPi = juce::MathConstants<float>::twoPi;
        const float srFloat = (float) sr;

        // ---- per-block coefficients ----
        const float delaySec = 0.04f + 0.0026f * juce::jlimit (0.0f, 100.0f, p.size); // 40..300 ms
        const float rt60     = juce::jlimit (0.05f, 20.0f, p.decay);

        // Feedback gain that yields the requested RT60 for one loop pass:
        //   G = 10^(-3 * delay / RT60) = exp(-6.9078 * delay / RT60)
        float fbBase = std::exp (-6.907755f * delaySec / rt60);
        fbBase = juce::jlimit (0.01f, 0.99f, fbBase);

        const float dampA = std::exp (-twoPi * juce::jlimit (20.0f, srFloat * 0.45f, p.damping) / srFloat);
        const float shimAmt = juce::jlimit (0.0f, 1.0f, p.shimmer / 12.0f);
        const float shiftRatio = std::pow (2.0f, juce::jlimit (0.0f, 12.0f, p.shimmer) / 12.0f);
        const float mix = juce::jlimit (0.0f, 1.0f, p.mix / 100.0f);
        const float depth = juce::jlimit (0.0f, 1.0f, p.depth / 100.0f);
        const float lfoInc = twoPi * juce::jlimit (0.01f, 20.0f, p.rate) / srFloat;

        const float hpA = 1.0f - std::exp (-twoPi * 120.0f / srFloat);
        const float lpB = 1.0f - std::exp (-twoPi * 8000.0f / srFloat);

        const float grainLen = juce::jmax (8.0f, 0.04f * srFloat);   // 40 ms grains
        const float shiftBase = juce::jmax (4.0f, 0.02f * srFloat);  // 20 ms minimum read distance

        const int lineN = (int) chans[0].line.size();
        const int delayLen = juce::jlimit (2, lineN - 2, (int) (delaySec * sr));

        for (int i = 0; i < n; ++i)
        {
            lfoPhase += lfoInc;
            if (lfoPhase >= twoPi)
                lfoPhase -= twoPi;

            const float mod = 1.0f + depth * 0.12f * std::sin (lfoPhase);
            const float fb = juce::jmin (0.995f, fbBase * mod); // hard stability clamp

            for (int ch = 0; ch < numCh; ++ch)
            {
                auto& c = chans[(size_t) ch];
                auto* data = buffer.getWritePointer (ch);
                const float in = data[i];
                const int thisLineN = (int) c.line.size();

                // comb read + damping low-pass inside the loop
                int rIdx = c.w - delayLen;
                if (rIdx < 0)
                    rIdx += thisLineN;

                const float delayed = c.line[(size_t) rIdx];
                c.damp = delayed * (1.0f - dampA) + c.damp * dampA;

                // keep a short history of the (damped) loop signal for the pitch shifter
                const int shimN = (int) c.shim.size();
                c.shim[(size_t) c.sw] = c.damp;

                // granular pitch shifter: two overlapping raised-cosine grains
                float shifted = c.damp;
                if (shimAmt > 0.0f)
                {
                    const float t0 = c.grain;
                    float t1 = t0 + 0.5f;
                    if (t1 >= 1.0f)
                        t1 -= 1.0f;

                    const float d0 = shiftBase + t0 * grainLen * shiftRatio;
                    const float d1 = shiftBase + t1 * grainLen * shiftRatio;

                    float w0 = std::sin (t0 * juce::MathConstants<float>::pi);
                    float w1 = std::sin (t1 * juce::MathConstants<float>::pi);
                    w0 *= w0;
                    w1 *= w1; // sin^2(t) + sin^2(t + 1/2) == 1 -> seamless crossfade

                    shifted = readAt (c.shim, c.sw, d0) * w0 + readAt (c.shim, c.sw, d1) * w1;
                }

                c.grain += 1.0f / grainLen;
                if (c.grain >= 1.0f)
                    c.grain -= 1.0f;

                // feedback: crossfade between the plain loop and the pitch-shifted copy
                const float fbSig = c.damp * (1.0f - shimAmt) + shifted * shimAmt;
                c.line[(size_t) c.w] = in + fbSig * fb;

                // wet output filters
                float wet = c.damp;
                if (p.hpf)
                {
                    c.hpState += hpA * (wet - c.hpState);
                    wet -= c.hpState;
                }
                if (p.lpf)
                {
                    c.lpState += lpB * (wet - c.lpState);
                    wet = c.lpState;
                }

                data[i] = (mix <= 0.0f) ? in : in * (1.0f - mix) + wet * mix;

                if (++c.w >= thisLineN)
                    c.w = 0;
                if (++c.sw >= shimN)
                    c.sw = 0;
            }
        }
    }

private:
    struct Channel
    {
        std::vector<float> line; // comb delay line
        std::vector<float> shim; // history used by the pitch shifter
        int w = 0;               // write index into line
        int sw = 0;              // write index into shim
        float damp = 0.0f;       // damping LPF state
        float hpState = 0.0f;    // wet HPF state
        float lpState = 0.0f;    // wet LPF state
        float grain = 0.0f;      // granular pitch-shifter phase (0..1)
    };

    static float readAt (const std::vector<float>& buf, int writeIdx, float dist)
    {
        const int n = (int) buf.size();
        float rp = (float) writeIdx - dist;
        rp = std::fmod (rp, (float) n);
        if (rp < 0.0f)
            rp += (float) n;

        const int i0 = (int) rp;
        const float frac = rp - (float) i0;
        const int i1 = (i0 + 1 >= n) ? 0 : i0 + 1;

        return buf[(size_t) i0] * (1.0f - frac) + buf[(size_t) i1] * frac;
    }

    std::vector<Channel> chans;
    double sr = 48000.0;
    float lfoPhase = 0.0f;
};
