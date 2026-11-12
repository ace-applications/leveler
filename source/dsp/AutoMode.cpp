#include "AutoMode.h"
#include <algorithm>
#include <cmath>

void AutoMode::setActive (bool b) noexcept
{
    if (b == active) return;
    active = b;
    if (!b) { target = 0.0f; effective.store (0.0f, std::memory_order_relaxed); }
}

void AutoMode::update (const Advisor::Result& r, double t)
{
    const float dt = lastUpdate < -1e8 ? 0.25f : (float) std::clamp (t - lastUpdate, 0.01, 0.5);
    lastUpdate = t;
    if (!active) { effective.store (0.0f, std::memory_order_relaxed); return; }

    const auto dir = (Advisor::Direction) r.direction;

    // inertia: a new direction must persist (Advisor already hysteresizes ~2 s) before
    // the intensity target flips — prevents rapid left/right oscillation on bursts.
    if (dir != committedDir)
        committedDir = dir;

    // dynamic difference -> magnitude
    const float mag = r.dynDb >= 18.0f ? 1.0f : r.dynDb >= 12.0f ? 0.7f : 0.35f;
    target = dir == Advisor::left  ? -mag * r.confidence
           : dir == Advisor::right ?  mag * r.confidence
           : 0.0f;

    // long smoothing (tau ~ 2 s) — no chatter, gradual adaptation
    const float coef = std::exp (-dt / 2.0f);
    const float cur = effective.load (std::memory_order_relaxed);
    effective.store (cur + coef * (target - cur), std::memory_order_relaxed);
}
