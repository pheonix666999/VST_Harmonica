# Turbo Tuning - Implementation Plan

## Product Vision
A simple, clean, kid-friendly blow-and-draw harmonica interface using a basic wind sensor controller.
Designed for harmonica and woodwind-style players with future MIDI integration potential.

**Target audience:** Kids, hobbyists, harmonica lovers, woodwind/brass enthusiasts
**Platform priority:** macOS (AU + VST3), Windows (VST3), Standalone
**Philosophy:** Simple, scalable, fun — build it right, step by step

---

## Phase 1: Interactive Demo (Current)
**Goal:** Working demo to show Jim (Turbo-Harp / MIDI harp box concept)

### Features
- [x] Major Diatonic Harp interface (10 holes, blow/draw)
- [x] Circle of Fifths key selector (all 12 major keys)
- [x] Instrument categories: Brass, Strings, Woodwinds
- [x] EQ (4-band: Low, Mid, High, Presence)
- [x] Effects (Reverb, Delay, Chorus)
- [x] Auto-Tune correction (kid-friendly mode)
- [x] Safe Mode (compressor/limiter — prevents bad sounds)
- [x] Keyboard + mouse interaction
- [x] Premium dark UI with glassmorphism design

### Tech Stack (Demo)
- HTML5 / CSS3 / Vanilla JavaScript
- Web Audio API for synthesis
- No dependencies — single folder, open in browser

---

## Phase 2: JUCE Plugin Foundation
**Goal:** Port demo to production JUCE/C++ plugin

### Structure
```
plugin/
├── CMakeLists.txt
├── Source/
│   ├── PluginProcessor.h/cpp      — Audio processing
│   ├── PluginEditor.h/cpp         — UI shell
│   ├── DSP/
│   │   ├── HarmonicaEngine.h/cpp  — Core harmonica synthesis
│   │   ├── BrassEngine.h/cpp      — Brass voice synthesis
│   │   ├── StringEngine.h/cpp     — String voice synthesis
│   │   ├── WoodwindEngine.h/cpp   — Woodwind voice synthesis
│   │   ├── AutoTuner.h/cpp        — Pitch correction
│   │   ├── SafeMode.h/cpp         — Compressor/limiter
│   │   └── EffectsChain.h/cpp     — Reverb, delay, chorus
│   ├── UI/
│   │   ├── TurboLookAndFeel.h     — Custom JUCE look and feel
│   │   ├── HarmonicaView.h/cpp    — Harmonica hole visualization
│   │   ├── CircleOfFifths.h/cpp   — Key selector widget
│   │   ├── InstrumentPanel.h/cpp  — Instrument category browser
│   │   └── EffectsPanel.h/cpp     — EQ + effects controls
│   └── Util/
│       ├── MusicTheory.h          — Key, scale, tuning tables
│       └── WindController.h/cpp   — Wind sensor MIDI mapping
├── Assets/
│   └── (UI graphics, fonts)
└── build_mac.sh / build_windows.bat
```

### Build Targets
- macOS: AU, VST3, Standalone
- Windows: VST3, Standalone

---

## Phase 3: Wind Controller Integration
**Goal:** MIDI input from wind sensor hardware

- Map breath pressure to velocity/dynamics
- Map lip position to hole selection
- Bend detection for expressive playing
- Calibration UI for different controllers
- Compatible with: Turbo-Harp, standard MIDI breath controllers

---

## Phase 4: Sound Design & Polish
**Goal:** Production-quality sounds

- Sample-based harmonica voices (optional hybrid approach)
- Physical modeling for brass/woodwinds
- Expanded instrument library
- Preset system (beginner, intermediate, pro presets)
- Visual feedback: waveform display, tuning meter

---

## Phase 5: MIDI Output & Integration
**Goal:** Use as a MIDI controller

- MIDI note output from harmonica interface
- MPE support for expressive controllers
- DAW integration (Logic, Ableton, GarageBand)
- MIDI learn for custom mappings

---

## Harmonica Tuning Reference

### Standard Richter Tuning (Key of C)
```
Hole:   1    2    3    4    5    6    7    8    9    10
Blow:   C4   E4   G4   C5   E5   G5   C6   E6   G6   C7
Draw:   D4   G4   B4   D5   F5   A5   B5   D6   F6   A6
```

### Circle of Fifths (Semitone Offsets)
```
C=0, G=7, D=2, A=9, E=4, B=11, F#=6, Db=1, Ab=8, Eb=3, Bb=10, F=5
```

### Relative Minors
```
C→Am, G→Em, D→Bm, A→F#m, E→C#m, B→G#m
F#→D#m, Db→Bbm, Ab→Fm, Eb→Cm, Bb→Gm, F→Dm
```

---

## Design Principles
1. **Kid-friendly** — Big buttons, bright feedback, forgiving auto-tune
2. **Simple first** — Core harmonica works perfectly before adding extras
3. **Scalable** — Architecture supports future instruments and features
4. **Mac-first** — Optimized for macOS (AU format priority)
5. **Visual delight** — Premium look that makes you want to play
6. **Safe Mode always available** — One button to sound good instantly
