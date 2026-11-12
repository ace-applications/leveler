# ACE Audio Leveler

One knob. Two directions. Dynamic control. — bidirectional real-time leveler plugin.

- **Left (−)**: loud parts lose dominance; quiet material untouched.
- **0**: bit-transparent pass-through.
- **Right (+)**: quiet details and tails come forward; transients preserved.

Free = manual knob + true bypass. Pro adds the **Direction Advisor** and **Live Adaptive
Auto Mode** (runtime `LicenseState` seam — always-valid stub until the ACE Studios
account-login unlock replaces it; no auth code in v1).

## Build (Windows, VS 2022)

```bash
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
./build/Release/leveler_selftest.exe
```

Outputs: `build/LevelerFree_artefacts/Release/VST3/Leveler Free.vst3`,
`build/LevelerPro_artefacts/Release/VST3/Leveler Pro.vst3`.
JUCE 8.0.15 fetched via FetchContent on first configure.

## Editions

Two binaries from one codebase, `LEVELER_EDITION_PRO` compile flag; Pro-only DSP modules
(`Advisor`, `AutoMode`) are absent from the Free binary. CI (`.github/workflows/release.yml`,
windows-2022) builds both, runs the DSP self-test, and zips on `v*` tags.

## Docs

- `docs/ACE Audio Leveler  Programmer Brief v1.3.md` — the contract (grill-locked).
- `docs/ACE AUDIO Leveler.png` — brand system (Obsidian Black / Electric Purple / Isometra + Inter).
- `PRODUCT.md` — product context.
- DSP internals are documented in the brief; constants are developer discretion.
