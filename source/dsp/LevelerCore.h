#pragma once
#include <atomic>
#include <cmath>

// Bidirectional dynamics core (brief v1.3 §4).
// One knob, intensity n in [-1, +1]:
//   n < 0 (LEFT): fast envelope exceeding the slow body envelope is pulled toward it
//         (gain = (fast/slow)^n) — loud parts lose dominance, quiet parts untouched.
//   n > 0 (RIGHT): fast envelope below the body envelope is lifted toward it — quiet
//         parts rise, transients (fast >= slow) untouched. Capped lift + noise-floor
//         guard prevent hiss/breath/room amplification.
//   n == 0: gain and makeup are forced to exactly 1.0 — bit-transparent center.
// Ratio-based detection makes the result invariant to input gain staging (normalization b);
// the loudness-matching makeup stage keeps overall level constant while the knob moves
// (normalization a). Stereo: envelopes from summed channel energy, one shared gain.
class LevelerCore
{
public:
    void prepare (double sampleRate, int numChannels);
    void setIntensity (float n);          // -1..+1, smoothed internally
    void setAutoActive (bool b) noexcept; // disables loudness-match recentering creep
    void process (float* const* channels, int numChannels, int numSamples);

    // introspection for the UI (thread-safe-ish reads)
    float appliedGainDb() const noexcept { return appliedGainDb_.load (std::memory_order_relaxed); }

    static float softClip (float x) noexcept; // ceiling 0 dBFS, knee starts at ~ -0.9 dBFS

private:
    struct Env
    {
        void prepare (double sr, double attackMs, double releaseMs);
        float process (float x) noexcept; // one-pole, fast attack / slow release
        float value = 0.0f;
        float upCoef = 0.0f, downCoef = 0.0f;
    };

    double sampleRate = 48000.0;
    float intensity = 0.0f;      // target
    float intensitySm = 0.0f;    // smoothed intensity (tau ~ 60 ms)
    float gain = 1.0f;           // smoothed applied gain
    float makeup = 1.0f;         // smoothed loudness-match output trim
    float meanGainDb = 0.0f;     // slow mean of gain in dB, tau ~ 1.5 s
    float autoActive = 0.0f;
    Env fastEnv, refEnv;
    float rmsSq = 0.0f;
    float silenceHold = 1.0f;
    std::atomic<float> appliedGainDb_ { 0.0f };
};
