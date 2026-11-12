# ACE Audio Leveler — Programmer Brief

Doc Version: 1.3 · Status: Functional Specification (grill-locked) · Target: VST3, Windows (v1)

v1.3 supersedes v1.1 and v1.2. Changes from v1.2 came out of a decisions session and are marked **[v1.3]** where new.

## 1. CORE PRODUCT CONCEPT

Real-time dynamics plugin, one central knob, three regions:

| Knob region | Audible behavior | DSP direction |
|---|---|---|
| LEFT (negative) | Loud parts less dominant; quiet parts unchanged as practical | Downward compression / adaptive gain reduction on high envelope |
| CENTER (0) | Unprocessed, transparent pass-through | Neutral path, seamless crossfade |
| RIGHT (positive) | Quiet details/tails more audible; transients preserved | Upward compression / adaptive gain lift on low envelope |

Distinct from ACE Audio LEVELIX (genre-aware mixing recommendations). Leveler changes dynamics; LEVELIX advises levels.

## 2. PACKAGING **[v1.3]**

- **Two binaries from one codebase**: `LevelerFree`, `LevelerPro` — same build recipe as KeySync (edition compile flag).
- Pro features (Advisor + Auto Mode) sit behind a **runtime `LicenseState` check**. The Pro binary compiles with an always-valid stub. Free binary excludes the module entirely.
- **Future**: single binary + online account login via ACE Studios API (does not exist yet — no auth code in v1, stub interface only). Swapping stub validation source must not touch feature code.
- Identity: manufacturer **ACE Audio** (code `Acea`), bundles `com.acea.LevelerFree` / `com.acea.LevelerPro`, plugin codes `LvlF` / `LvlP`.
- v1 ships **Windows VST3 only**. AU/macOS, AAX are roadmap items, not v1 acceptance. Code signing deferred (see KeySync SmartScreen note).

## 3. KNOB SPEC **[v1.3]**

- User-facing scale: **−12 … +12 dB**, 0.5 dB steps, 0 = neutral. Displayed in dB on the UI.
- Scale means **dynamic-range change**, not output gain.
- **Both normalizations**:
  1. Level-independent detector — processing result is invariant to input gain staging.
  2. Loudness-matched output compensation — turning the knob does not shift overall loudness.

## 4. DSP REQUIREMENTS

- **Zero latency** — no lookahead. **[v1.3]**
- Transient preservation: sidechain high-pass and/or transient detection; attacks stay punchy at moderate settings.
- Gain smoothing: variable attack/release envelope tracking; no clicks, zipper noise, modulation distortion, or rhythmic pumping.
- Stereo link: equal gain across channels; no image tilt or center drift. Mono instances collapse cleanly.
- Noise-floor protection: cap upward lift on low-level signals (hiss, breath, room). Near-silence (< −60 dBFS) → neutral/unity.
- Output protection **[v1.3]**: internal headroom ceiling + soft-knee soft-clipper engaged only when peaks exceed 0 dBFS (mainly under RIGHT lift). No lookahead, no undersampling; slight smear only when clipper engages. Documented in DSP spec.
- Sample rates 44.1–192 kHz. Stable on silence, DC offset, clipped input, pre-compressed masters.
- Internal constants (window lengths, thresholds, time-constants, curve shapes) are **developer discretion**, documented in the DSP spec deliverable.

## 5. PRO FEATURE A — DIRECTION ADVISOR

Real-time analysis; recommends LEFT / RIGHT / NO CHANGE with:
- Dynamic-difference rating (Low / Moderate / High) from PLR / crest analysis.
- Confidence score.
- Short explanation string ("High transient dominance detected").

Stability: no jitter or chaotic jumping on isolated hits; intentionally dynamic material must not false-positive.

## 6. PRO FEATURE B — LIVE ADAPTIVE AUTO MODE

Rolling short-term window → classify direction → compute intensity → smooth transition → continuous adaptation.
- Hysteresis/inertia against left/right oscillation.
- Noise-floor protection and zero-signal → neutral.
- **UI coupling [v1.3]**: the knob visually follows the engine's current intensity. Grabbing the knob while Auto is on switches to manual (Auto toggles off). Disabling Auto returns to manual knob position with smooth ramping.
- Auto state and effective intensity always visible.

## 7. UI **[v1.3]**

Brand system (per design sheet `docs/ACE AUDIO Leveler.png`):
- Obsidian Black `#080808` background · Pure White `#FFFFFF` · Neutral Grey `#A3A3A3` · **Electric Purple `#B18CFF`** accent (live/active/chosen only).
- Title/wordmark in **Isometra** (title use only — body glyphs broken), body in **Inter**. Fonts SIL OFL, bundled as binary data.
- Waveform logo from the brand sheet; glass panels simulated (low-alpha white fill + hairline border, KeySync pattern).
- One big central knob as hero control; Advisor readout (Pro) below; Auto + Bypass beside it.
- Left = less loud dominance, right = more quiet detail — understandable without a manual.

### Parameters **[v1.3]**

Three automatable parameters, full state save/restore, smooth ramps on automation:
1. Level (−12…+12 dB)
2. Bypass
3. Auto toggle (Pro)

### Bypass **[v1.3]**

Single button: ~10 ms ramped transition into a true-bypass path (DSP silent, near-zero CPU). No clicks entering or exiting.

## 8. TESTING & ACCEPTANCE

Test matrix from v1.2 stands: drums, vocals, melodic sources, time-based FX, problem signals (silence, hiss, pre-compressed, clipped), across knob positions.

**CI selftest [v1.3]**: KeySync recipe — console test target runs seeded synthetic signals through the DSP core offline and asserts envelope statistics (transient punch, noise-lift caps, ceiling behavior, neutral-center transparency). Runs in CI before release zips.

Acceptance checklist (from v1.2, still binding):
- LEFT reduces relative peak dominance, not overall level.
- RIGHT increases quiet-detail audibility, not uniform gain.
- Center = transparent pass-through; bypass comparison reliable.
- Advisor stable with uncertainty metrics; Auto has zero erratic hunting.
- Zero clicks, zipper noise, unwanted clipping, pumping.

## 9. DELIVERY

- DSP specification document (curves, filters, detector time-constants, clipper behavior).
- `LevelerFree` / `LevelerPro` VST3 binaries (win-x64), zipped per KeySync release flow.
- Clean repo: CMake + JUCE 8 (FetchContent), build scripts, GitHub Actions release workflow on `v*` tags.
- Benchmark data: CPU load across sample rates. (No latency to report — zero-latency.)
- Repeatable test sessions / audio examples for left / center / right.

## 10. OUT OF SCOPE (v1)

- Genre target curves (LEVELIX territory).
- Exposed attack/release/ratio/threshold controls.
- Presets. **[v1.3]**
- I/O metering and level-matched A/B button. **[v1.3]**
- Auth / license validation code. **[v1.3]**
- macOS (AU), AAX. **[v1.3]**
- Project-wide track balancing.
- Perfect noise/audio separation.

## 11. ROADMAP (post-v1)

1. Account login unlock (ACE Studios API) → single binary.
2. macOS AU build + signing.
3. AAX (requires Avid agreement + PACE).
4. I/O metering, level-matched comparison, presets — if user demand appears.
