// DSP regression tests for ReverbEngine.
// Build with:  cmake -B build -DDREAMY_BUILD_TESTS=ON
// Run with:    ./build/ReverbTests

#include <juce_audio_basics/juce_audio_basics.h>
#include "ReverbEngine.h"

#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

static int failures = 0;

#define CHECK(cond, msg)                                                        \
    do {                                                                        \
        if (cond) { std::printf("  ok   : %s\n", msg); }                        \
        else      { std::printf("  FAIL : %s\n", msg); ++failures; }            \
    } while (0)

namespace
{
    struct RunResult
    {
        float peak = 0.0f;
        float headRms = 0.0f;
        float tailRms = 0.0f;
        bool finite = true;
        std::vector<float> output; // first channel, when collect == true
    };

    // mode: 0 = silence, 1 = unit impulse at sample 0, 2 = white noise
    RunResult run (const ReverbEngine::Parameters& p,
                   double seconds,
                   int mode,
                   int blockSize,
                   bool collect = false,
                   double sr = 48000.0,
                   int channels = 2)
    {
        ReverbEngine engine;
        engine.prepare (sr, blockSize, channels);

        juce::ScopedNoDenormals noDenormals;

        juce::AudioBuffer<float> buf (channels, blockSize);
        std::mt19937 rng (12345);
        std::normal_distribution<float> gauss (0.0f, 0.25f);

        RunResult r;
        const long long totalSamples = (long long) (seconds * sr);
        const long long headEnd = (long long) (0.5 * sr);
        const long long tailStart = totalSamples - (long long) (0.5 * sr);

        double headE = 0.0; long long headN = 0;
        double tailE = 0.0; long long tailN = 0;

        if (collect)
            r.output.reserve ((size_t) totalSamples);

        long long done = 0;
        while (done < totalSamples)
        {
            const int n = (int) std::min ((long long) blockSize, totalSamples - done);
            buf.clear();

            if (mode == 1 && done == 0)
            {
                for (int ch = 0; ch < channels; ++ch)
                    buf.setSample (ch, 0, 1.0f);
            }
            else if (mode == 2)
            {
                for (int i = 0; i < n; ++i)
                {
                    const float v = gauss (rng);
                    for (int ch = 0; ch < channels; ++ch)
                        buf.setSample (ch, i, v);
                }
            }

            engine.process (buf, p);

            for (int i = 0; i < n; ++i)
            {
                const float v = buf.getSample (0, i);
                if (! std::isfinite (v))
                    r.finite = false;
                r.peak = std::max (r.peak, std::abs (v));

                const long long s = done + i;
                if (s < headEnd) { headE += (double) v * v; ++headN; }
                if (s >= tailStart) { tailE += (double) v * v; ++tailN; }

                if (collect)
                    r.output.push_back (v);
            }

            done += n;
        }

        r.headRms = headN > 0 ? (float) std::sqrt (headE / headN) : 0.0f;
        r.tailRms = tailN > 0 ? (float) std::sqrt (tailE / tailN) : 0.0f;
        return r;
    }
}

int main()
{
    std::printf("DreamyReverb DSP tests\n");

    // ---------------------------------------------------------------
    std::printf("\n[1] mix = 0 must be bit-exact passthrough\n");
    {
        ReverbEngine engine;
        engine.prepare (48000.0, 128, 2);

        ReverbEngine::Parameters p;
        p.mix = 0.0f;

        juce::AudioBuffer<float> buf (2, 128);
        for (int i = 0; i < 128; ++i)
        {
            buf.setSample (0, i, std::sin (i * 0.1f) * 0.5f);
            buf.setSample (1, i, std::cos (i * 0.13f) * 0.5f);
        }

        std::vector<float> in0, in1;
        for (int i = 0; i < 128; ++i) { in0.push_back (buf.getSample (0, i)); in1.push_back (buf.getSample (1, i)); }

        engine.process (buf, p);

        bool same = true;
        for (int i = 0; i < 128; ++i)
            if (buf.getSample (0, i) != in0[(size_t) i] || buf.getSample (1, i) != in1[(size_t) i])
                same = false;

        CHECK (same, "output equals input exactly");
    }

    // ---------------------------------------------------------------
    std::printf("\n[2] silence in -> silence out (no self-oscillation)\n");
    {
        ReverbEngine::Parameters p;
        p.decay = 10.0f; p.size = 100.0f; p.shimmer = 12.0f;
        p.depth = 100.0f; p.rate = 8.0f; p.mix = 100.0f;

        auto r = run (p, 5.0, 0, 512);
        CHECK (r.finite && r.peak == 0.0f, "output stays exactly zero for 5 s");
    }

    // ---------------------------------------------------------------
    std::printf("\n[3] short decay tail dies out\n");
    {
        ReverbEngine::Parameters p;
        p.decay = 0.5f; p.size = 50.0f; p.shimmer = 0.0f; p.mix = 100.0f;

        auto r = run (p, 3.0, 1, 512);
        CHECK (r.finite, "output finite");
        CHECK (r.tailRms < 1.0e-6f, "tail at 3 s is dead (< -120 dBFS)");
    }

    // ---------------------------------------------------------------
    std::printf("\n[4] long decay tail is still alive at 3 s\n");
    {
        ReverbEngine::Parameters p;
        p.decay = 8.0f; p.size = 50.0f; p.shimmer = 0.0f; p.mix = 100.0f;

        auto r = run (p, 3.0, 1, 512);
        CHECK (r.finite, "output finite");
        CHECK (r.tailRms > 1.0e-7f && r.tailRms > r.headRms * 1.0e-4f,
               "tail at 3 s still carries energy");
    }

    // ---------------------------------------------------------------
    std::printf("\n[5] worst-case params + noise: stable for 15 s\n");
    {
        ReverbEngine::Parameters p;
        p.decay = 10.0f; p.size = 0.0f;      // shortest loop, longest decay
        p.shimmer = 12.0f;                    // max pitch shift in the loop
        p.depth = 100.0f; p.rate = 4.0f;      // max modulation
        p.damping = 100.0f;                   // min damping
        p.mix = 100.0f; p.hpf = true; p.lpf = true;

        auto r = run (p, 15.0, 2, 512);
        CHECK (r.finite, "no NaN/Inf over 15 s");
        CHECK (r.peak < 60.0f, "peak stays bounded");
    }

    // ---------------------------------------------------------------
    std::printf("\n[6] output must not depend on host block size\n");
    {
        ReverbEngine::Parameters p;
        p.decay = 6.0f; p.size = 85.0f; p.damping = 4500.0f;
        p.shimmer = 9.0f; p.mix = 65.0f; p.depth = 75.0f; p.rate = 1.3f;
        p.hpf = true; p.lpf = false;

        auto a = run (p, 2.0, 2, 4096, true);
        auto b = run (p, 2.0, 2, 97, true);

        CHECK (a.finite && b.finite, "both runs finite");

        float maxDiff = 0.0f;
        const size_t n = std::min (a.output.size(), b.output.size());
        for (size_t i = 0; i < n; ++i)
            maxDiff = std::max (maxDiff, std::abs (a.output[i] - b.output[i]));

        CHECK (n > 0, "samples were produced");
        CHECK (maxDiff < 1.0e-5f, "block sizes 4096 vs 97 give the same output");
    }

    // ---------------------------------------------------------------
    std::printf("\n[7] mono processing is safe\n");
    {
        ReverbEngine::Parameters p;
        auto r = run (p, 2.0, 1, 256, false, 48000.0, 1);
        CHECK (r.finite && r.peak > 0.0f, "mono impulse processed without issues");
    }

    // ---------------------------------------------------------------
    std::printf("\n[8] shimmer on/off both stay stable over 10 s of noise\n");
    {
        ReverbEngine::Parameters p;
        p.decay = 9.0f; p.size = 100.0f; p.mix = 100.0f;
        p.shimmer = 0.0f;
        auto r0 = run (p, 10.0, 2, 512);
        CHECK (r0.finite && r0.peak < 60.0f, "shimmer = 0 stable");

        p.shimmer = 12.0f;
        auto r1 = run (p, 10.0, 2, 512);
        CHECK (r1.finite && r1.peak < 60.0f, "shimmer = 12 stable");
    }

    std::printf("\n%s (%d failure%s)\n",
                failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED",
                failures, failures == 1 ? "" : "s");

    return failures == 0 ? 0 : 1;
}
