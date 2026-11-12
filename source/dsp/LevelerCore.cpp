#include "LevelerCore.h"
#include <algorithm>

namespace
{
    constexpr float silenceFloorLin  = 0.001f;   // -60 dBFS: neutral/unity below this
    constexpr float noiseGuardLin    = 0.00562f; // -45 dBFS body level: start limiting lift
    constexpr float maxLiftLin       = 3.981f;   // +12 dB hard cap on upward lift
    constexpr float maxDuckLin       = 0.251f;   // -12 dB hard cap on downward pull
    constexpr float silenceHoldCoef  = 0.999986f; // hold halves in ~1 s at 48k (hysteresis
                                                  // on entering silence)
    constexpr float leftExponentK    = 0.55f;    // log-domain strength per unit intensity

    float dB (float x) noexcept  { return 20.0f * std::log10 (std::max (x, 1.0e-9f)); }
    float lin (float db) noexcept { return std::pow (10.0f, db / 20.0f); }
}

void LevelerCore::Env::prepare (double sr, double attackMs, double releaseMs)
{
    upCoef   = (float) std::exp (-1000.0 / (sr * attackMs));  // rising toward a bigger value
    downCoef = (float) std::exp (-1000.0 / (sr * releaseMs)); // falling toward a smaller value
}

float LevelerCore::Env::process (float x) noexcept
{
    // upCoef/downCoef are retention factors (exp(-1/tau_samples)); the per-sample
    // step is their complement.
    const float c = 1.0f - (x > value ? upCoef : downCoef);
    value += c * (x - value);
    return value;
}

float LevelerCore::softClip (float x) noexcept
{
    const float t = 0.9f; // knee start (~ -0.9 dBFS)
    const float a = std::abs (x);
    if (a <= t) return x;
    const float sign = x < 0.0f ? -1.0f : 1.0f;
    return sign * (t + (1.0f - t) * std::tanh ((a - t) / (1.0f - t)));
}

void LevelerCore::prepare (double sr, int /*numChannels*/)
{
    sampleRate = sr;
    fastEnv.prepare (sr, 2.0, 40.0);      // peak-ish: very quick attack (must catch the
                                          // first milliseconds of a transient), short
                                          // release so quiet material between hits is
                                          // never ducked
    refEnv.prepare (sr, 10.0, 8000.0);    // recent-peak reference: rises fast on loud
                                          // content (must catch transients), decays over
                                          // seconds — the target the RIGHT direction lifts
                                          // quiet material toward
    rmsSq = 0.0f;
    gain = 1.0f; makeup = 1.0f; meanGainDb = 0.0f;
    intensitySm = intensity; appliedGainDb_.store (0.0f);
}

void LevelerCore::setIntensity (float n)
{
    intensity = std::clamp (n, -1.0f, 1.0f);
}

void LevelerCore::setAutoActive (bool b) noexcept { autoActive = b ? 1.0f : 0.0f; }

void LevelerCore::process (float* const* channels, int numChannels, int numSamples)
{
    const float intensCoef = std::exp (-1.0f / (0.06f * (float) sampleRate));
    const float gainAtk    = std::exp (-1000.0 / (sampleRate * 2.0));    // fast catch (must
                                                                         // duck the first ms
                                                                         // of a transient)
    const float gainRel    = std::exp (-1000.0 / (sampleRate * 40.0));   // quick recover
    const float meanCoef   = std::exp (-1.0 / (1.5 * sampleRate));
    const float makeupCoef = std::exp (-1.0 / (0.25 * sampleRate));

    for (int i = 0; i < numSamples; ++i)
    {
        // --- envelopes (stereo-linked: summed channel energy) ---
        float e = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            e += channels[ch][i] * channels[ch][i];
        const float mono = std::sqrt (e / (float) std::max (numChannels, 1));

        const float fast = fastEnv.process (mono);
        refEnv.process (mono);
        rmsSq += (1.0f - meanCoef) * (mono * mono - rmsSq);
        const float slow = std::sqrt (std::max (rmsSq, 1.0e-12f));

        // --- intensity smoothing ---
        intensitySm += (1.0f - intensCoef) * (intensity - intensitySm);
        const float n = intensitySm;

        // --- silence hysteresis: once quiet, stay neutral until clearly above floor ---
        silenceHold = (slow < silenceFloorLin ? silenceHold * silenceHoldCoef : 1.0f);

        float target = 1.0f;
        if (n != 0.0f && silenceHold > 0.5f)
        {
            const float r = fast / std::max (slow, 1.0e-9f);
            if (n < 0.0f)
            {
                if (r > 1.0f)
                    target = std::clamp (std::pow (r, n * leftExponentK), maxDuckLin, 1.0f);
            }
            else
            {
                // RIGHT: lift quiet material toward the recent-peak reference,
                // gated off while a transient is playing (never boost hits).
                const bool gate = fast < slow * 1.5f;
                const float refRatio = refEnv.value / std::max (slow, 1.0e-6f);
                float lift = std::clamp (std::pow (std::max (refRatio, 1.0f), n * 0.35f),
                                         1.0f, maxLiftLin);
                // noise-floor guard: taper the lift by ABSOLUTE body level — full
                // lift above -45 dBFS, none at -60 dBFS — so hiss/room/breath
                // near the floor is never "leveled" upward into audibility.
                const float liftScale = std::clamp ((dB (slow) + 60.0f) / 15.0f, 0.0f, 1.0f);
                lift = 1.0f + (lift - 1.0f) * liftScale;
                if (slow < noiseGuardLin)
                    lift = std::min (lift, 1.12f); // near-silent floor: tiny lift max
                target = gate ? lift : 1.0f;
            }
        }

        // --- gain smoothing: fast down, slow up ---
        const float gc = 1.0f - (target < gain ? gainAtk : gainRel);
        gain += gc * (target - gain);

        // --- loudness-match makeup: invert the slowly-averaged applied gain ---
        if (n == 0.0f && autoActive == 0.0f)
        {
            meanGainDb = 0.0f;
            makeup = 1.0f; // bit-transparent center
        }
        else
        {
            meanGainDb += (1.0f - meanCoef) * (dB (gain) - meanGainDb);
            // loudness match at 70% strength: full compensation would cancel the core
            // relative-dynamics change (the body dominates the loudness average);
            // 70% keeps overall level near-constant while the effect stays audible.
            const float match = 0.7f * (autoActive > 0.5f ? 1.0f : std::abs (n));
            makeup += (1.0f - makeupCoef) * (lin (-meanGainDb * match) - makeup);
        }

        appliedGainDb_.store (dB (gain), std::memory_order_relaxed);

        // --- apply (soft-clip ceiling always armed: identity below the knee, so
        // transparent material is untouched; catches makeup overshoot on any knob side) ---
        const float g = gain * makeup;
        for (int ch = 0; ch < numChannels; ++ch)
            channels[ch][i] = softClip (channels[ch][i] * g);
    }
}
