#pragma once
#include <atomic>
#include <type_traits>

// Pro Feature A: Direction Advisor (brief v1.3 §5).
// Audio thread: cheap per-frame RMS/peak accumulation into a 3 s ring.
// Message thread: evaluate() computes crest/spread/tail statistics, applies
// hysteresis, publishes an atomic Result. 4 Hz cadence is plenty.
class Advisor
{
public:
    enum Direction : int { none = 0, left = 1, right = 2 };

    struct Result
    {
        int direction = none;
        float confidence = 0.0f;   // 0..1
        float dynDb = 0.0f;        // dynamic spread (p90-p10 of short-term RMS)
        const char* explanation = "";
    };

    void prepare (double sampleRate);
    void processSample (float monoSample) noexcept; // audio thread
    void evaluate() { evaluate (nowSeconds()); }    // message thread
    void evaluate (double tSeconds);                // time-injectable (selftest)
    Result result() const noexcept { return snap.load (std::memory_order_relaxed); }

private:
    struct Frame { float rmsDb = -120.0f; float peakDb = -120.0f; };
    static_assert (std::is_trivially_copyable_v<Frame>);

    static constexpr int frameSamples = 2400; // 50 ms @ 48 k
    static constexpr int numFrames = 60;      // 3 s window

    double sampleRate = 48000.0;
    // audio thread commits, message thread reads — relaxed atomics keep evaluate()
    // free of torn frames (a torn read would only garble one 4 Hz evaluation cycle)
    std::atomic<Frame> frames[numFrames] = {};
    int writeIdx = 0;
    double sampleCounter = 0;
    float frameSumSq = 0.0f;
    float framePeak = 0.0f;
    int pendingDirection = none;
    double pendingSince = -1e9;
    std::atomic<Result> snap { Result{} };

    static double nowSeconds();
};
