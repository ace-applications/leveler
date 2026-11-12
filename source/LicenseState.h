#pragma once
// Runtime Pro-feature seam (brief v1.3 §2).
// v1: the Pro binary compiles this stub with an always-valid license; the Free binary
// excludes the Pro DSP modules entirely. Swapping to ACE Studios account login later
// means replacing the stub's validation source — feature code does not change.
struct LicenseState
{
    static bool proUnlocked() { return true; } // stub: always valid until account login exists
};
