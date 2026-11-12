#include "Advisor.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <vector>

double Advisor::nowSeconds()
{
    return std::chrono::duration<double> (
               std::chrono::steady_clock::now().time_since_epoch()).count();
}

void Advisor::prepare (double sr)
{
    sampleRate = sr;
    for (auto& f : frames) f.store (Frame{}, std::memory_order_relaxed);
    writeIdx = 0; sampleCounter = 0; frameSumSq = 0; framePeak = 0;
    pendingSince = -1e9;
    pendingDirection = none;
    snap.store (Result{});
}

void Advisor::processSample (float x) noexcept
{
    frameSumSq += x * x;
    framePeak = std::max (framePeak, std::abs (x));
    if (++sampleCounter >= (double) frameSamples)
    {
        frames[writeIdx].store (
            { 20.0f * std::log10 (std::sqrt (frameSumSq / (float) frameSamples) + 1e-9f),
              20.0f * std::log10 (framePeak + 1e-9f) },
            std::memory_order_relaxed);
        writeIdx = (writeIdx + 1) % numFrames;
        frameSumSq = 0; framePeak = 0; sampleCounter = 0;
    }
}

void Advisor::evaluate (double t)
{

    std::vector<float> rmsDb, peakDb;
    rmsDb.reserve (numFrames); peakDb.reserve (numFrames);
    for (const auto& f : frames)
    {
        const auto v = f.load (std::memory_order_relaxed);
        rmsDb.push_back (v.rmsDb);
        peakDb.push_back (v.peakDb);
    }
    std::sort (rmsDb.begin(), rmsDb.end());
    std::sort (peakDb.begin(), peakDb.end());
    auto pct = [&] (const std::vector<float>& v, float p)
    {
        return v[std::clamp ((int) (p * (float) (v.size() - 1) + 0.5f), 0, (int) v.size() - 1)];
    };
    const float spread = pct (rmsDb, 0.9f) - pct (rmsDb, 0.1f);
    const float p50    = pct (rmsDb, 0.5f);
    // window PLR: window peak vs combined RMS — the metric that actually separates
    // transient-dominant material from steady sources (per-frame crest cannot).
    float meanSq = 0.0f;
    for (float v : rmsDb) meanSq += std::pow (10.0f, v / 10.0f);
    const float windowRmsDb = 10.0f * std::log10 (meanSq / (float) numFrames + 1e-12f);
    const float windowPeakDb = peakDb.back();
    const float plr = windowPeakDb - windowRmsDb; // peak-to-loudness ratio over the window
    // transient excess: how far the loudest short-term frames sit above the typical
    // body level — survives sparse transients where p90/p10 collapse onto body frames
    const float transientExcess = pct (rmsDb, 0.95f) - p50;

    Result r;
    if (p50 < -60.0f)
    {
        r = { none, 1.0f, spread, "" };
    }
    else
    {
        const float tailFrac = [&]
        {
            int c = 0;
            for (float v : rmsDb) if (v < p50 - 12.0f) ++c;
            return (float) c / (float) numFrames;
        }();

        int raw = none;
        // Intentionally dynamic material must not false-positive: transient EXCESS
        // above the body level (not raw spread) is what marks peak dominance.
        if (plr >= 12.0f && transientExcess >= 10.0f)       raw = left;
        else if (tailFrac >= 0.25f && plr >= 12.0f)         raw = right;

        if (raw == pendingDirection)
        {
            if (raw != none && t - pendingSince >= 2.0) // sustained bias before committing
            {
                if (raw != result().direction)
                    pendingSince = t;
                r.direction = raw;
                r.confidence = raw == left
                    ? std::clamp ((plr - 12.0f) / 12.0f + (transientExcess - 10.0f) / 12.0f, 0.15f, 1.0f)
                    : std::clamp ((tailFrac - 0.25f) / 0.5f + (plr - 12.0f) / 10.0f, 0.15f, 1.0f);
                r.explanation = raw == left ? "High transient dominance detected"
                                            : "Sustained tail buried";
            }
            else
            {
                r.direction = result().direction; // hold current until bias sustains
                r.confidence = result().confidence * 0.9f;
                r.explanation = result().explanation;
            }
        }
        else
        {
            pendingDirection = raw;
            pendingSince = t;
            r.direction = result().direction;
            r.confidence = result().confidence * 0.9f;
            r.explanation = result().explanation;
        }

        if (r.direction == left)  r.explanation = "High transient dominance detected";
        if (r.direction == right) r.explanation = "Sustained tail buried";
    }

    r.dynDb = spread;
    snap.store (r, std::memory_order_relaxed);
}
