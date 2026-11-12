#pragma once
#include "Advisor.h"

// Pro Feature B: Live Adaptive Auto Mode (brief v1.3 §6).
// Consumes Advisor results at control rate; maps dynamic difference to a signed
// intensity target, applies inertia (sustained direction before target changes are
// mirrored) and a long smoothing time-constant, and publishes the effective signed
// intensity the LevelerCore consumes. The UI knob follows this value.
class AutoMode
{
public:
    void setActive (bool b) noexcept;
    void update (const Advisor::Result& r, double timeSeconds); // call ~4 Hz on message thread
    float effectiveIntensity() const noexcept { return effective.load (std::memory_order_relaxed); }

private:
    bool active = false;
    Advisor::Direction committedDir = Advisor::none;
    double lastUpdate = -1e9;
    std::atomic<float> effective { 0.0f };
    float target = 0.0f;
};
