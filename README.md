# wir3d // PROTOCOL 07

<div align="center">

```
  ██     ██ ██ ██████  ██████  ██████  
  ██     ██ ██ ██   ██      ██ ██   ██ 
  ██  █  ██ ██ ██████   █████  ██   ██ 
  ██ ███ ██ ██ ██   ██      ██ ██   ██ 
   ███ ███  ██ ██   ██ ██████  ██████  
```

**Cyberpunk Vocal Pitch Shifter, Scale Autotune, Harmonizer Ping-Pong Delay & Glitch Repeat Unit**  
*Inspired by Serial Experiments Lain & The Matrix Aesthetic*

[![Format](https://img.shields.io/badge/Format-VST3%20%7C%20Standalone-00ff66?style=flat-square)](https://github.com/)
[![Framework](https://img.shields.io/badge/Framework-JUCE%208-00ff66?style=flat-square)](https://juce.com/)
[![C++](https://img.shields.io/badge/C%2B%2B-17-00ff66?style=flat-square)](https://en.cppreference.com/)
[![License](https://img.shields.io/badge/License-MIT-00ff66?style=flat-square)](LICENSE)

</div>

---

## ⚡ Overview

**wir3d** is an anime cyberpunk vocal effect processor developed with **JUCE 8** and modern C++17. Designed for live and studio production, it blends surgical real-time scale pitch correction with creative spatial and rhythmic glitch effects.

---

## 🔮 Key Features

* **High-Precision YIN Pitch Detection**: Sliding-window tracking running every 128 samples (~2.9 ms) with sub-sample parabolic interpolation and subharmonic de-octaving (divisors 2, 3, and 4) to eliminate octave jumping.
* **Target Locking & Hysteresis**: Prevents vibrato flutter by locking to scale notes with intelligent hysteresis thresholds.
* **Speed Response**:
  * **92% – 100%**: Instantaneous hard robotic quantize (0 ms snap, T-Pain / Cher style).
  * **0% – 91%**: Exponential glide (2 ms to 350 ms) for natural, transparent portamento.
* **Pitch-Synchronous Adaptive Overlap-Add**: Dynamic grain sizing ($2.5 \times T_0$) avoids phase flanging and metallic artifacts.
* **HARMO DELAY**: Secondary fifth harmony (+7 semitones) fed into a dedicated stereo Ping-Pong cross-delay (Left ~180 ms $\leftrightarrow$ Right ~360 ms) with warm analog tape filtering.
* **BEAT REPEAT & Glitch Filter**: Real-time slice capture and looping (`1/4`, `1/8`, `1/16`, `1/32`, and `Glitch` 25 ms) with dedicated Dry/Wet control and resonant cyber bandpass filter.
* **SMILE Macro**: Rising high-pass sweep, atmospheric echo bloom, and soft tube/analog saturation.
* **Master Filter**: Bipolar master tone shaper (Left: warm Low-Pass 20 kHz $\to$ 200 Hz; Right: crisp High-Pass 20 Hz $\to$ 3.8 kHz).
* **Multi-Scale Quantizer**: Chromatic, Major, Minor, Major Pentatonic, Minor Pentatonic, Blues, Dorian, Phrygian, and Mixolydian.
* **Vocal Range Selector**: Wide (All), Low (Bass/Bari), Mid (Tenor/Alto), and High (Soprano).
* **Preset System**: Factory presets + user preset saving directly from the UI.
* **Matrix / Lain UI**: Custom CRT scanline aesthetic, phosphor laser green meters, and vector Lain Iwakura cyberpunk mascot.

---

## 🎛 Controls

| Control | Range | Description |
|---|---|---|
| **BEND** | -12 to +12 st | Continuous manual pitch transposition. |
| **TUNE** | 0 to 100% | Scale correction depth. |
| **SPEED** | 0 to 100% | Transition speed: smooth glides (0%) to hard snap (100%). |
| **HARMO** | 0 to 100% | Harmonizer (+7 st) ping-pong delay wet mix. |
| **SMILE** | 0 to 100% | High-pass bloom, space echo and tube saturation macro. |
| **RPT MIX** | 0 to 100% | Beat repeat loop dry/wet blend. |
| **DIVISION** | Choice | Beat repeat slice division (`1/4`, `1/8`, `1/16`, `1/32`, `Glitch`). |
| **FILTER** | -100 to +100 | Master bipolar filter: LP (`< 0`) $\leftrightarrow$ HP (`> 0`). Center is bypass. |
| **KEY / SCALE** | Choice | Target musical root key and scale. |
| **RANGE** | Choice | Optimizes YIN tracking for specific vocal ranges. |
| **PRESET / SAVE**| Choice / Btn | Instant patch recall and user preset persistence. |

---

## 🛠 Building from Source

### Requirements
* [CMake 3.22+](https://cmake.org/download/)
* [Git](https://git-scm.com/)
* C++17 compatible compiler:
  * **Windows**: Visual Studio 2022 (with *Desktop development with C++*)
  * **macOS**: Xcode 14+ / Clang
  * **Linux**: GCC 10+ / Clang 12+

### Quick Build (Windows PowerShell)

```powershell
# Clone or navigate to the repository
cd "wir3d"

# Configure CMake
cmake -B build -S . -G "Visual Studio 17 2022" -A x64

# Compile Release build
cmake --build build --config Release
```

The compiled VST3 binary will be generated at:
```
build/WIR3D_artefacts/Release/VST3/wir3d.vst3
```

To install on Windows:
Copy `wir3d.vst3` into `C:\Program Files\Common Files\VST3`.

### Quick Build (macOS / Linux)

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j$(nproc 2>/dev/null || sysctl -n hw.ncpu)
```

---

## 📁 Project Structure

```
wir3d/
├── CMakeLists.txt          # Modern CMake configuration fetching JUCE 8
├── README.md               # Documentation
├── LICENSE                 # MIT License
└── Source/
    ├── PitchDetector.h     # High accuracy YIN pitch tracking with de-octaving
    ├── PitchDetector.cpp
    ├── ScaleQuantizer.h    # Full musical scale quantizer
    ├── ScaleQuantizer.cpp
    ├── PluginProcessor.h   # Audio DSP engine, ping-pong delay, beat repeat
    ├── PluginProcessor.cpp
    ├── PluginEditor.h      # LookAndFeel & Matrix/Lain UI definition
    └── PluginEditor.cpp
```

---

## 📜 License

Distributed under the **MIT License**. See `LICENSE` for details.
