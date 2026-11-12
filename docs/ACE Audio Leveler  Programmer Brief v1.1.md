ACE AUDIO 

# **AUDIO LEVELER** 

Programmer Brief · Product Concept v1.1 

**ONE KNOB. TWO DIRECTIONS. DYNAMIC CONTROL.** 

### **Purpose** 

ACE Audio Leveler is a simple bidirectional dynamic-level control for individual audio tracks. With the knob centered at 0, turning left reduces loud parts relative to quieter material; turning right raises quiet parts relative to louder material. The aim is to control dynamic range while keeping the workflow immediate and musical. 

**Document status:** This brief defines intended behavior. The developer should propose and validate the DSP architecture before implementation. 

ACE AUDIO  |  AUDIO LEVELER  |  PROGRAMMER BRIEF 

1 

## **1. Product Overview** 

ACE Audio Leveler is a real-time audio plugin for individual tracks and instruments inside a DAW. Its defining feature is one central knob with neutral position at 0 and two distinct processing directions. 

- **Turn left (negative):** reduce louder portions/peaks relative to quieter material. The quiet parts should be affected as little as practical. 

- **Center (0):** neutral, unprocessed signal. 

- **Turn right (positive):** lift the weakest/quietest parts relative to louder material. The loudest peaks and transients should remain mostly unchanged. 

Keep this distinct from ACE Audio LEVELIX, which recommends genre-aware instrument and vocal levels. Leveler changes the dynamics of the incoming signal; LEVELIX provides mixing-level guidance. 

## **2. Main Knob Behavior** 

|**Knob position**|**Expected behavior**|**Technical direction**|
|---|---|---|
|Left / negative|Loud parts become less dominant; quieter material is<br>preserved as much as possible.|Downward compression or adaptive<br>gain reduction.|
|Center / 0|Original audio; no intentional processing.|Unity/neutral path. Bypass should be<br>available.|
|Right / positive|Quiet parts and tails become more audible; loud<br>peaks and transients remain mostly unchanged.|Upward compression or adaptive<br>gain control.|



The knob should control a musical intensity parameter mapped internally to suitable DSP settings. Do not expose threshold, ratio, attack, release, or makeup gain as required user controls in the first version. 

## **3. DSP / Processing Requirements** 

A single ordinary gain control cannot achieve this behavior. The plugin needs dynamic processing that responds differently to quieter and louder portions of the signal. The developer should choose the most robust architecture and document its trade-offs. 

- **Negative range:** use downward compression or adaptive gain reduction to reduce louder sections relative to quieter content. 

- **Positive range:** use upward compression or adaptive gain control to raise quieter sections relative to loud peaks. 

- **Transient awareness:** preserve the punch of kick, snare, plucks, and other attacks, especially at moderate settings. 

- **Gain smoothing:** avoid clicks, zipper noise, and obvious pumping through suitable envelope tracking and smoothing. 

- **Noise awareness:** positive leveling can raise room noise, hiss, breaths, and other low-level content. Limit excessive noise lift and behave sensibly on silence/near-silence. 

- **Output safety:** include sufficient internal headroom and a strategy to prevent unexpected clipping. Protection should not unnecessarily flatten transients. 

- **Stereo linking:** link gain behavior appropriately for stereo material to avoid unstable image movement. 

ACE AUDIO  |  AUDIO LEVELER  |  PROGRAMMER BRIEF 

2 

## **4. Signal Handling and Stability** 

- Process audio in real time and support agreed sample rates and buffer sizes. 

- Handle mono and stereo signals correctly. 

- Remain stable with silence, near-silence, DC offsets, clipped audio, and extreme input levels. 

- Avoid parameter-change clicks and unreasonable CPU use. 

- If lookahead or oversampling is used, document latency and CPU impact. 

## **5. User Interface Requirements** 

- One primary knob with a clear 0 center position and negative/positive ranges. 

- The left and right directions should be understandable without reading a manual: left reduces loud parts; right lifts quiet parts. 

- Provide a true bypass control. 

- Input/output metering is desirable but secondary to the one-knob workflow. 

- A level-matched comparison option is desirable. 

- No genre selector is required for the core product. 

## **6. Minimum Viable Product (MVP)** 

|**Requirement**|**Acceptance expectation**|
|---|---|
|Bidirectional knob|Negative positions reduce loud parts; positive positions lift quiet parts; 0 is neutral.|
|Transient-aware response|Common drum and pluck attacks retain useful punch at moderate settings.|
|Safe output|No unexpected clipping in normal use; protection behavior is documented.|
|Bypass / comparison|User can compare processed audio with the original reliably.|
|Stable real-time operation|No crashes, severe artifacts, or unreasonable CPU usage in supported DAWs.|



ACE AUDIO  |  AUDIO LEVELER  |  PROGRAMMER BRIEF 

3 

## **7. Test Plan and Acceptance Criteria** 

Test several knob positions (left, center, right, and intermediate values) on a representative set of material. 

- **Drums:** kick, snare, hi-hats, percussion, and loops. Check transient punch and loud-hit control. 

- **Melodic sources:** plucks, pads, piano, guitar, synth leads, and sustained sounds. Check quieter notes and tails. 

- **Vocals:** loud/quiet words and phrases. Check intelligibility, natural expression, and breath/noise lift. 

- **Effects:** reverb and delay tails. Check audibility and pumping. 

- **Problem inputs:** silence, very low-level audio, noisy recordings, already-compressed audio, and clipped audio. 

### **Acceptance criteria** 

- Moving left should reduce the relative dominance of louder parts, rather than merely turning the whole signal down. 

- Moving right should increase the relative audibility of quiet parts, rather than merely turning the whole signal up. 

- At moderate settings, important transients should remain perceptually punchy. 

- The center position should be neutral and bypass comparison should be reliable. 

- The plugin should avoid obvious clicks, severe pumping, and excessive noise lift in normal use. 

## **8. Technical Delivery** 

- Propose the DSP architecture and explain how both knob directions work. 

- Deliver plugin build, source code, build instructions, dependencies, and licensing notes. 

- Agree supported formats and DAWs; VST3 on Windows is a sensible first target to evaluate. 

- Provide repeatable test sessions or audio examples showing left, center, and right behavior. 

- Document latency, CPU use, limitations, and output-protection behavior. 

## **9. Out of Scope for Version 1** 

- Genre-based instrument target recommendations (ACE Audio LEVELIX). 

- A complex multi-control compressor interface. 

- Automatic processing of every track in a DAW project. 

- Claims of perfect separation between quiet musical details and background noise. 

**Success definition:** One knob centered at 0. Turn left to make loud parts less dominant; turn right to bring the weakest parts forward. The result should feel more controlled while retaining musical punch, with no complicated setup. 

ACE AUDIO  |  AUDIO LEVELER  |  PROGRAMMER BRIEF 

4 

