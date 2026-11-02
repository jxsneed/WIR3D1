WIR3D — Cyberpunk Vocal Processor for Windows (VST3 & Standalone)

DESCRIPTION
wir3d is a futuristic vocal processing unit inspired by Serial Experiments Lain
and the Matrix aesthetic, combining:
- Ultra-accurate fundamental vocal tracking (YIN pitch detection algorithm).
- Hard-snap / robotic quantizer and smooth vocal glide (SPEED).
- Harmonizer Ping-Pong Delay (wide stereo +7th fifth harmonies bouncing L <-> R).
- SMILE: High-pass bloom, echo space and tube saturation macro.
- BEAT REPEAT: Rhythmic glitch looper (1/4, 1/8, 1/16, 1/32, Glitch) with dedicated Dry/Wet.
- MASTER FILTER: Bipolar Low-Pass (< 0) to High-Pass (> 0) master tone shaper.
- PRESET MANAGER: Built-in cyber presets + user preset save capability.
- KEY & SCALE quantizer: Chromatic, Major, Minor, and Minor Pentatonic.

CONTROLS
BEND: Continuous manual pitch transpose (-12 to +12 semitones).
TUNE: Amount of scale-based pitch correction (0 to 100%).
SPEED: Auto-tune snap response. Above 92% is hard instantaneous quantize (T-Pain/Cher/robotic snap);
       below 92% is natural vocal transition glide.
HARMO DELAY: Blends harmonized voice (+7 semitones) into stereo ping-pong cross-delay.
SMILE: Rising high-pass, ambient echo bloom and soft tube/analog saturation.
RPT MIX: Beat repeat dry/wet blend.
REPEAT: Loop slice division (1/4, 1/8, 1/16, 1/32, Glitch).
MASTER FILTER: Center (0) is bypass. Left is warm analog Low-Pass filter; Right is crisp High-Pass filter.
KEY / SCALE: Sets the target vocal pitch scale (including Pentatonic for cyber hooks).
PRESET / SAVE: Instant recall of curated factory patches and custom user slots.

BUILD INSTRUCTIONS (WINDOWS)
1. Install Visual Studio 2022 Community ("Desktop development with C++").
2. Install CMake 3.22+ and Git.
3. Open PowerShell in this folder and run:

   cmake -B build -S . -G "Visual Studio 17 2022" -A x64
   cmake --build build --config Release

4. The compiled VST3 will be located at:
   build/WIR3D_artefacts/Release/VST3/wir3d.vst3

5. Copy wir3d.vst3 to:
   C:\Program Files\Common Files\VST3

6. Rescan plug-ins in your DAW (Ableton Live, FL Studio, Reaper, Cubase, etc.).
