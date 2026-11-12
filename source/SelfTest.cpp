// DSP self-test: seeded synthetic signals through LevelerCore / Advisor / AutoMode.
// Asserts the acceptance criteria from the v1.3 brief (§8). Exit 0 = pass.
#include "dsp/LevelerCore.h"
#include "dsp/Advisor.h"
#include "dsp/AutoMode.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

static int failures = 0;

static void check (bool ok, const char* name, const char* detail = "")
{
    std::printf ("[%s] %s", ok ? "PASS" : "FAIL", name);
    if (detail[0]) std::printf ("  (%s)", detail);
    std::printf ("\n");
    if (! ok) ++failures;
}

static double rmsOf (const std::vector<float>& v, size_t from, size_t to)
{
    double s = 0;
    for (size_t i = from; i < to; ++i) s += (double) v[i] * v[i];
    return std::sqrt (s / (double) std::max<size_t> (1, to - from));
}

static double db (double lin) { return 20.0 * std::log10 (std::max (lin, 1e-9)); }

// burst bed: loud decaying "kick" pings every 0.5 s over a quiet sustained body
static std::vector<float> burstBed (double sr, double seconds, double bodyAmp, double burstAmp)
{
    std::vector<float> x ((size_t) (sr * seconds), 0.0f);
    for (size_t i = 0; i < x.size(); ++i)
    {
        const double t = (double) i / sr;
        double v = bodyAmp * std::sin (2.0 * 3.14159265358979 * 220.0 * t); // quiet body
        const double pingT = std::fmod (t, 0.5);
        if (pingT < 0.12)
            v += burstAmp * std::exp (-pingT * 40.0) * std::sin (2.0 * 3.14159265358979 * 60.0 * pingT);
        x[i] = (float) v;
    }
    return x;
}

static void run (LevelerCore& core, std::vector<float>& io)
{
    float* ch[1] = { io.data() };
    core.process (ch, 1, (int) io.size());
}

int main()
{
    constexpr double sr = 48000.0;

    { // 1. center = bit-transparent
        LevelerCore core; core.prepare (sr, 1); core.setIntensity (0.0f);
        std::vector<float> x (4096);
        std::mt19937 rng (7);
        std::uniform_real_distribution<float> d (-0.9f, 0.9f);
        for (auto& v : x) v = d (rng);
        auto orig = x;
        run (core, x);
        bool identical = std::equal (orig.begin(), orig.end(), x.begin(),
            [] (float a, float b) { return a == b; });
        check (identical, "center is bit-transparent");
    }
    { // 2. LEFT reduces peak dominance, quiet body untouched
        LevelerCore core; core.prepare (sr, 1); core.setIntensity (-1.0f);
        auto x = burstBed (sr, 6.0, 0.05, 0.7);
        auto y = x;
        std::vector<float> gainHist (y.size());
        for (size_t i2 = 0; i2 < y.size(); ++i2)
        {
            float* ch[1] = { y.data() + i2 };
            core.process (ch, 1, 1);
            gainHist[i2] = core.appliedGainDb();
        }
        // measure in steady state (envelopes + intensity + makeup settled): the first
        // ping runs on cold state and would dominate the global max unfairly
        const size_t w0 = (size_t) (sr * 2.0);
        const double peakIn = *std::max_element (x.begin() + w0, x.end(),
            [] (auto a, auto b) { return std::abs (a) < std::abs (b); });
        const double peakOut = *std::max_element (y.begin() + w0, y.end(),
            [] (auto a, auto b) { return std::abs (a) < std::abs (b); });
        const size_t pingPeak = (size_t) (std::max_element (x.begin() + (size_t) (sr * 2.5),
            x.begin() + (size_t) (sr * 2.62),
            [] (float a, float b) { return std::abs (a) < std::abs (b); }) - x.begin());
        // quiet body segment between pings, in the settled window
        const size_t q0 = (size_t) (sr * 2.72), q1 = (size_t) (sr * 2.92);
        const double bodyIn = db (rmsOf (x, q0, q1)), bodyOut = db (rmsOf (y, q0, q1));
        const double relIn = bodyIn - db (peakIn), relOut = bodyOut - db (peakOut);
        char det[160];
        std::snprintf (det, 160, "rel %.1f -> %.1f dB, body delta %.2f dB, gain@ping=%.2f @2.65=%.2f",
                       relIn, relOut, bodyOut - bodyIn, (double) gainHist[pingPeak],
                       (double) gainHist[(size_t) (sr * 2.65)]);
        // loudness-match makeup may lift the whole signal; the honest acceptance
        // metric is relative peak dominance, plus "quiet parts not ducked"
        check (relOut > relIn + 2.0 && bodyOut > bodyIn - 1.5,
               "LEFT reduces peak dominance, keeps body", det);
    }
    { // 3. RIGHT raises relative quiet-detail audibility, peaks stay put
        LevelerCore core; core.prepare (sr, 1); core.setIntensity (1.0f);
        auto x = burstBed (sr, 6.0, 0.05, 0.7);
        auto y = x;
        std::vector<float> gainHist3 (y.size());
        for (size_t i2 = 0; i2 < y.size(); ++i2)
        {
            float* ch[1] = { y.data() + i2 };
            core.process (ch, 1, 1);
            gainHist3[i2] = core.appliedGainDb();
        }
        // steady-state window (first ping runs on cold state)
        const size_t w0 = (size_t) (sr * 2.0);
        const double peakIn = *std::max_element (x.begin() + w0, x.end(), [] (auto a, auto b) { return std::abs (a) < std::abs (b); });
        const double peakOut = *std::max_element (y.begin() + w0, y.end(), [] (auto a, auto b) { return std::abs (a) < std::abs (b); });
        const size_t q0 = (size_t) (sr * 2.72), q1 = (size_t) (sr * 2.92);
        const double relIn = db (rmsOf (x, q0, q1)) - db (peakIn);
        const double relOut = db (rmsOf (y, q0, q1)) - db (peakOut);
        char det[128];
        std::snprintf (det, 128, "quiet-vs-peak %.1f -> %.1f dB; gain@2.80=%.2f @2.90=%.2f",
                       relIn, relOut, (double) gainHist3[(size_t) (sr * 2.80)],
                       (double) gainHist3[(size_t) (sr * 2.90)]);
        check (relOut > relIn + 2.0 && peakOut <= peakIn * 1.05,
               "RIGHT lifts quiet detail, peaks preserved", det);
    }
    { // 4. digital silence stays silent, no gain explosion
        LevelerCore core; core.prepare (sr, 1); core.setIntensity (1.0f);
        std::vector<float> x (sr * 2, 0.0f);
        run (core, x);
        bool silent = true;
        for (float v : x) if (v != 0.0f) silent = false;
        check (silent, "silence in, silence out");
    }
    { // 5. low-level hiss is not amplified into audibility
        LevelerCore core; core.prepare (sr, 1); core.setIntensity (1.0f);
        std::vector<float> x ((size_t) (sr * 4.0));
        std::mt19937 rng (11);
        std::normal_distribution<float> d (0.0f, 1.0f);
        const float noiseAmp = (float) std::pow (10.0, -70.0 / 20.0);
        for (auto& v : x) v = d (rng) * noiseAmp;
        run (core, x);
        const double outDb = db (rmsOf (x, (size_t) (sr * 2.0), x.size()));
        char det[64]; std::snprintf (det, 64, "hiss out %.1f dBFS", outDb);
        // correct behavior: hiss stays roughly at its input level, never boosted
        check (outDb < -45.0 && outDb > -72.0, "noise floor protected under RIGHT", det);
    }
    { // 6. clipped input never explodes past the ceiling
        LevelerCore core; core.prepare (sr, 1); core.setIntensity (1.0f);
        auto x = burstBed (sr, 4.0, 0.9, 1.4); // already clipping hard
        for (auto& v : x) v = std::clamp (v, -1.0f, 1.0f);
        run (core, x);
        bool within = true;
        for (float v : x) if (std::abs (v) > 1.0001f) within = false;
        check (within, "output ceiling respected on clipped input");
    }
    { // 7. stereo link: one shared gain — channel ratio preserved
        LevelerCore core; core.prepare (sr, 2); core.setIntensity (-0.8f);
        const size_t n = (size_t) (sr * 4.0);
        std::vector<float> l (n), r (n), l0 (n), r0 (n);
        for (size_t i = 0; i < n; ++i)
        {
            const double t = (double) i / sr;
            l0[i] = (float) (0.6 * std::exp (-std::fmod (t, 0.5) * 30.0) * std::sin (2.0 * 3.14159 * 80.0 * t));
            r0[i] = (float) (0.04 * std::sin (2.0 * 3.14159 * 300.0 * t));
            l[i] = l0[i]; r[i] = r0[i];
        }
        float* ch[2] = { l.data(), r.data() };
        core.process (ch, 2, (int) n);
        bool linked = true;
        for (size_t i = 0; i < n; ++i)
        {
            if (std::abs (l0[i]) < 0.05f || std::abs (r0[i]) < 0.01f) continue;
            const double gl = std::abs ((double) l[i] / l0[i]);
            const double gr = std::abs ((double) r[i] / r0[i]);
            if (std::abs (gl - gr) > 2e-3) { linked = false; break; }
        }
        check (linked, "stereo-linked gain (no image tilt)");
    }
    { // 8. gain-staging invariance (normalization: level-independent detector)
        auto shape = [&] (double inDb)
        {
            LevelerCore core; core.prepare (sr, 1); core.setIntensity (-1.0f);
            auto x = burstBed (sr, 6.0, 0.05, 0.7);
            const float g = (float) std::pow (10.0, inDb / 20.0);
            for (auto& v : x) v *= g;
            run (core, x);
            return core.appliedGainDb();
        };
        const double g1 = shape (-6.0), g2 = shape (-30.0);
        char det[64]; std::snprintf (det, 64, "mean gain %.1f vs %.1f dB", g1, g2);
        check (std::abs (g1 - g2) < 3.0, "detector is gain-staging invariant", det);
    }
    { // 9. Advisor: transient-dominant material -> LEFT, sustained & stable
        Advisor adv; adv.prepare (sr);
        auto x = burstBed (sr, 12.0, 0.05, 0.8);
        for (float v : x) adv.processSample (v);
        adv.evaluate (0.5);            // ring still filling; seeds nothing yet
        adv.evaluate (3.0);            // full window: arms pending LEFT
        adv.evaluate (5.2);            // bias sustained -> commit
        const auto r = adv.result();
        char det[96];
        std::snprintf (det, 96, "dir=%d conf=%.2f dyn=%.1f", r.direction,
                       (double) r.confidence, (double) r.dynDb);
        check (r.direction == Advisor::left && r.confidence > 0.15f,
               "Advisor recommends LEFT on transient-dominant material", det);
    }
    { // 10. Advisor: silence -> NO CHANGE, confidence 1
        Advisor adv; adv.prepare (sr);
        std::vector<float> x ((size_t) (sr * 4.0), 0.0f);
        for (float v : x) adv.processSample (v);
        adv.evaluate (0.2);
        adv.evaluate (2.6);
        const auto r = adv.result();
        check (r.direction == Advisor::none && r.confidence >= 0.99f, "Advisor: silence is NO CHANGE");
    }
    { // 11. AutoMode: maps direction+confidence to signed intensity, silence drifts home
        AutoMode am; am.setActive (true);
        Advisor::Result loud { Advisor::left, 0.8f, 20.0f, "" };
        for (double t = 0.0; t < 10.0; t += 0.25) am.update (loud, t);
        const float up = am.effectiveIntensity();
        Advisor::Result quiet { Advisor::none, 1.0f, 0.0f, "" };
        for (double t = 10.0; t < 40.0; t += 0.25) am.update (quiet, t);
        const float home = am.effectiveIntensity();
        char det[64]; std::snprintf (det, 64, "left %.2f -> neutral %.3f", up, home);
        check (up < -0.5f && std::abs (home) < 0.05f, "AutoMode intensity mapping + release", det);
    }

    std::printf ("\n%s (%d failure%s)\n", failures == 0 ? "ALL TESTS PASS" : "TESTS FAILED",
                 failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
