# ACE Audio Leveler — PRODUCT.md

> Inferred by impeccable init from the v1.3 programmer brief + brand sheet + grilling decisions
> session (2026-11-12). No human interview was run beyond that session; assumptions are labeled
> where the brief is silent.

## Product

ACE Audio Leveler is a real-time audio dynamics plugin for individual tracks inside a DAW.
One central knob controls dynamic-range change in dB (−12…+12): left reduces the dominance of
loud parts, right lifts quiet details, center (0) is a transparent pass-through. Free and Pro
editions ship as separate Windows VST3 binaries; Pro adds the Direction Advisor and Live
Adaptive Auto Mode behind a runtime `LicenseState` seam (always-valid stub until the ACE
Studios account-login unlock replaces it).

## Audience

Music producers and mix engineers mid-session who want dynamic control without a
multi-knob compressor. Job: place the knob, hear control, move on. Success = more controlled
track that still has punch, with zero setup.

## Modes

- **Operate** — the plugin window is a tool: scanability and instant legibility outrank
  expression. Brand lives in precise details (wordmark lockup, purple accent, glass panels).

## Visual world

Per `docs/ACE AUDIO Leveler.png` (user-pinned canon):
- Obsidian Black `#080808` background; Pure White `#FFFFFF` ink; Neutral Grey `#A3A3A3` for
  secondary text/track arcs; **Electric Purple `#B18CFF` as the single accent** — live, active
  and chosen states only.
- Title lockup "Audio Leveler" in Isometra (title use only; body glyphs broken), body text in
  Inter Regular/SemiBold. Fonts bundled as binary data, SIL OFL.
- Waveform logo asset (`assets/logo.png`); glass panels simulated (low-alpha white fill +
  hairline border + top shine).
- Left = less loud dominance, right = more quiet detail — legible without a manual.

## Constraints (labeled assumptions)

- Assumption: knob follows the Auto engine's intensity; grabbing it turns Auto off (confirmed
  in grilling, brief §6).
- Assumption: Free window is shorter (no Pro panel) but same width and branding.
- Assumption: DSP internal constants are developer discretion, documented in the DSP spec.

## Explicitly out of scope (v1)

Presets; I/O metering; level-matched A/B button; auth code; macOS/AU/AAX; genre curves
(LEVELIX territory); project-wide balancing.
