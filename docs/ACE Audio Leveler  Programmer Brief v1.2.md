ACE AUDIO — SOFTWARE SPECIFICATION BRIEF 

# ACE Audio Leveler — Programmer Brief 

Doc Version: 1.2 Status: Functional Specification Target Platform: VST3 / AU / AAX (Win & macOS) 

Product Scope Distinction: ACE Audio Leveler directly changes the dynamic characteristics of incoming audio tracks in real time. Keep this product strictly distinct from ACE Audio LEVELIX, which provides genre-aware mixing level recommendations. 

## 1. CORE PRODUCT CONCEPT 

ACE Audio Leveler is a real-time audio dynamics plugin designed to control the dynamic range of individual tracks and instruments using one central knob. The control provides a continuous, smooth transition across three functional regions without step artifacts or gain clicks. 

|Knob Region|Expected Audible Behavior|Technical DSP Direction|
|---|---|---|
|LEFT / Negative<br>DOWNWARD RANGE|Reduces the dominance of loud parts relative to<br>quieter material. Quiet portions remain as<br>unchanged as practical.|Downward compression or adaptive gain<br>reduction targeting high envelope peaks.|
|CENTER / 0<br>NEUTRAL PATH|Original audio passes through unprocessed. Unity<br>gain, zero added coloration or dynamic alteration.|Clean bypass / neutral DSP path with<br>seamless transition capability.|
|RIGHT / Positive<br>UPWARD RANGE|Makes quieter details and tails more audible<br>relative to loud material, preserving transient<br>attacks and peaks.|Upward compression or adaptive gain lift<br>targeting low envelope levels.|



## 2. FREE EDITION VS. PRO ARCHITECTURE 

The product line is divided into a streamlined Free tier focused on manual workflow and a feature-rich Pro tier containing intelligent signal analysis and automated dynamic control. 

### Free Edition Requirements 

Provides a clean, manual single-knob workflow. Features include manual left/right leveling, a clearly marked neutral center position (0), downward leveling (left), upward leveling (right), hard/soft bypass toggle, and high-efficiency real-time processing. 

### Pro Feature A: Direction Advisor 

An intelligent real-time analysis module that evaluates incoming audio and displays a recommended processing direction (LEFT, RIGHT, or NO CHANGE), an estimated dynamic difference metric, a confidence score, and a concise textual explanation. 

### Pro Feature B: Live Adaptive Auto Mode 

An automated engine that continuous analyzes audio over a rolling time window to determine optimal processing direction and intensity, automatically applying gain adjustment with hysteresis-driven stability. 

### Core Operating Constraint 

The main knob controls dynamic processing intensity, not simple static output volume. Cross-center sweeps must remain artifact-free, avoiding pumping, clicks, excessive noise amplification, or transient flattening. 

ACE Audio | Leveler Programmer Brief v1.2 

Page 1 of 4 

## 3. PRO FEATURE SPECIFICATIONS 

Developers are expected to implement the Pro analysis engines using robust statistical sidechain evaluation while preserving low CPU overhead. 

## PRO FEATURE A: DIRECTION ADVISOR 

- Target Functionality: Analyzes signal energy distribution and dynamic range over time to guide user adjustments without forcing automated changes. 

- Recommendation States: 

   - LEFT — Loud peaks are disproportionately dominant relative to body material; downward leveling advised. 

   - RIGHT — Low-level details/tails are buried beneath loud peaks; upward leveling advised. 

   - NO CHANGE — Audio exhibits balanced dynamic proportions; no corrective leveling recommended. 

- Advisor Output Payload: 

   - Directional Recommendation: Explicit state indicator (LEFT / RIGHT / NO CHANGE). 

   - Dynamic Difference Estimation: Categorical rating (Low / Moderate / High) based on peak-to-Loudness (PLR) or crest factor analysis. 

   - Confidence Score: Quantitative certainty metric (e.g., percentage or Low/Medium/High bar). 

   - Explanation String: Short human-readable context (e.g., "High transient dominance detected" or "Sustained tail buried"). 

- Measurement Stability: Analysis must update continuously without jitter or chaotic jumping on isolated drum hits. Source material characteristics must be accounted for—a high dynamic difference alone must not trigger false positive recommendations on intentionally dynamic material. 

## PRO FEATURE B: LIVE ADAPTIVE AUTO MODE 

### Automated Execution Workflow: 

1. Continuously sample incoming audio energy across a calibrated short-term window (e.g., 300ms–1500ms). 

2. Classify structural signal dynamics to select the target direction (LEFT, RIGHT, or NO CHANGE). 

3. Calculate an optimal intensity value mapped to the processing core. 

4. Smoothly transition processing parameters to the calculated target. 

5. Maintain continuous adaptation as macro-dynamics change throughout playback. 

- Stabilization & Safety Mechanics: 

   - Hysteresis & Inertia: Require sustained directional bias before changing processing states to prevent rapid left/right oscillation during transient bursts. 

   - Noise Floor Protection: Limit upward gain lift on low-level signals to prevent amplification of hiss, breath noise, or room ambiance. 

   - Zero-Signal Handling: Immediately default to neutral/unity gain during silence or near-silence (< -60 dBFS). 

- User Control Integration: Auto Mode state and current effective intensity must be clearly displayed. Disabling Auto Mode instantly and predictably returns control to the manual knob position with smooth gain ramping. 

ACE Audio | Leveler Programmer Brief v1.2 

Page 2 of 4 

## 4. FREE VS. PRO FEATURE COMPARISON MATRIX 

|Plugin Feature / Capability|Free Edition|Pro Edition|
|---|---|---|
|Manual Left/Right Leveling Knob|Yes|Yes|
|Neutral Center Position at 0 (Unity Pass-through)|Yes|Yes|
|Downward Leveling (Left-side Range)|Yes|Yes|
|Upward Leveling (Right-side Range)|Yes|Yes|
|True Audio Bypass Control|Yes|Yes|
|Direction Advisor Module|No|Yes|
|Dynamic-Difference Signal Analysis|No|Yes|
|Recommendation Confidence & Explanation Metric|No|Yes|
|Automatic Direction Selection|No|Yes|
|Automatic Intensity Adjustment|No|Yes|
|Live Adaptive Auto Mode Engine|No|Yes|



## 5. TECHNICAL DSP & PROCESSING REQUIREMENTS 

Developers are encouraged to propose specific internal DSP topology (e.g., parallel upward/downward compression curves, adaptive RMS/peak detectors), provided all requirements below are met. 

### Transient Preservation Strategy 

Incorporate dedicated transient detection or sidechain high-pass filtering to allow primary attacks (drums, acoustic plucks) to pass intact at moderate settings before heavy gain attenuation or expansion kicks in. 

### Gain Smoothing & Anti-Pumping 

Implement variable attack/release envelope tracking or adaptive time-constants to eliminate zipper noise, modulation distortion on low frequencies, and intrusive rhythmic pumping. 

### Stereo Link & Channel Coherence 

Stereo instances must link channel energy detection appropriately to enforce equal gain reduction/expansion across Left and Right channels, preventing stereo image tilt or center-stage drifting. 

### Output Headroom & Clipping Protection 

Upward expansion inherently boosts ambient energy and can push peaks over 0 dBFS. Provide internal headroom management and a clean, non-smearing peak limiter on the output stage. 



<!-- Start of picture text -->
SYSTEM ARCHITECTURE BLOCK DIAGRAM<br><!-- End of picture text -->



<!-- Start of picture text -->
Pro: Advisor & Auto<br>Analysis Engine<br>Transient & Energy Bi-Directional DSP Output Safety Ceiling<br>Audio In<br>Detector Array Upward / Downward Limiter / Pass-through<br>Manual Knob Engine<br>Free & Pro Control<br><!-- End of picture text -->

ACE Audio | Leveler Programmer Brief v1.2 

Page 3 of 4 

6. TESTING PLAN & ACCEPTANCE CRITERIA 

The developer must validate plugin builds against the test matrix below using dedicated evaluation sessions across supported DAWs (Reaper, Pro Tools, Logic Pro, Ableton Live, Cubase). 

|Test Source Category|Test Material Description|Key Verification Criteria|
|---|---|---|
|Drums & Percussion|Isolated kick, snare, hi-hats, loops, acoustic and<br>electronic kits.|Ensure transient attacks stay sharp; no<br>heavy dulling at moderate settings.|
|Vocals|Dynamic lead vocals, spoken word, vocal<br>phrases with varied distance.|Verify consistent intelligibility; ensure<br>breath noise isn't overly lifted.|
|Melodic Instruments|Acoustic guitar plucks, sustained piano, synths,<br>orchestral pads.|Check sustain tails in RIGHT mode; verify<br>smooth decay withoutflutter.|
|Time-Based FX|Reverb tails, delay repeats, ambient room mics.|Evaluate tail audibility lift; check for<br>pumping on dense rhythmic input.|
|Problem Signals|Digital silence, low-level hiss, pre-compressed<br>master, clipped audio.|Verify stability; ensure zero gain<br>explosions orfilter blowing.|



## EXPLICIT ACCEPTANCE VERIFICATION CHECKLIST 

LEFT Motion: Reduces relative peak dominance without acting as a simple master volume fader. 

- RIGHT Motion: Increases quiet detail audibility without simple uniform gain amplification. CENTER (0): Perfectly neutral bit-accurate or transparent pass-through. 

- Advisor Module: Delivers reasonable, stable recommendations with clear uncertainty metrics. 

Auto Mode Engine: Selects logical direction and intensity with zero erratic hunting or chatter. 

- Artifact Safety: Zero severe clicks, zipper noise, unwanted digital clipping, or pumping. 

## 7. TECHNICAL DELIVERY & DEVELOPMENT SCOPE 

### Developer Deliverables 

- DSP Specification Document: Explanation of internal curves, filters, and detector time-constants. 

- Plugin Binaries: VST3, AU, AAX builds for Windows and macOS (Apple Silicon + Intel). 

- Source Code & Builds: Clean repository with build scripts and CMake/JUCE setup. 

- Benchmarking Data: Latency and CPU load measurements across sample rates (44.1k–192k). 

### Out of Scope for Version 1.2 

- Genre Target Curves: Explicitly reserved for ACE Audio LEVELIX. 

- Multi-Parameter Panel: No exposed manual controls for attack, release, or ratio. 

- Project Automation: No automatic global project-wide track balancing. 

- Absolute Noise Isolation: Perfect separation of noise from audio is not required. 

ACE Audio | Leveler Programmer Brief v1.2 

Page 4 of 4 

