#pragma once
#include <JuceHeader.h>
#include <array>
#include <map>
#include <string>

namespace TurboTuning
{

/**
 * Music theory constants used throughout the plugin.
 * Standard Richter-tuned diatonic harmonica layout.
 */
namespace MusicTheory
{
    // Standard Richter tuning in C — MIDI note numbers for holes 1–10
    constexpr std::array<int, 10> HARMONICA_BLOW = { 60, 64, 67, 72, 76, 79, 84, 88, 91, 96 };
    constexpr std::array<int, 10> HARMONICA_DRAW = { 62, 67, 71, 74, 77, 81, 83, 86, 89, 93 };

    // Circle of Fifths key names
    inline const std::array<juce::String, 12> KEYS = {
        "C", "G", "D", "A", "E", "B", "F#", "Db", "Ab", "Eb", "Bb", "F"
    };

    // Semitone offset from C for each key
    inline const std::map<juce::String, int> KEY_OFFSETS = {
        { "C", 0 }, { "G", 7 }, { "D", 2 }, { "A", 9 }, { "E", 4 }, { "B", 11 },
        { "F#", 6 }, { "Db", 1 }, { "Ab", 8 }, { "Eb", 3 }, { "Bb", 10 }, { "F", 5 }
    };

    // Relative minor for each major key
    inline const std::map<juce::String, juce::String> RELATIVE_MINORS = {
        { "C", "Am" }, { "G", "Em" }, { "D", "Bm" }, { "A", "F#m" },
        { "E", "C#m" }, { "B", "G#m" }, { "F#", "D#m" }, { "Db", "Bbm" },
        { "Ab", "Fm" }, { "Eb", "Cm" }, { "Bb", "Gm" }, { "F", "Dm" }
    };

    // Chromatic note names
    inline const std::array<juce::String, 12> NOTE_NAMES = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };

    /** Convert MIDI note number to frequency (A4 = 440 Hz). */
    inline float midiToFreq (int midi)
    {
        return 440.0f * std::pow (2.0f, (midi - 69) / 12.0f);
    }

    /** Get note name from MIDI number. */
    inline juce::String midiToNoteName (int midi)
    {
        return NOTE_NAMES[midi % 12] + juce::String (midi / 12 - 1);
    }

    /** Get the MIDI note for a hole, action, and key. */
    inline int getNote (int hole, bool isBlow, const juce::String& key)
    {
        jassert (hole >= 1 && hole <= 10);
        const auto& baseNotes = isBlow ? HARMONICA_BLOW : HARMONICA_DRAW;
        int baseMidi = baseNotes[(size_t)(hole - 1)];
        auto it = KEY_OFFSETS.find (key);
        int offset = (it != KEY_OFFSETS.end()) ? it->second : 0;
        return baseMidi + offset;
    }

} // namespace MusicTheory

/** Enumeration of instrument types */
enum class InstrumentType
{
    Harmonica = 0,
    Trumpet,
    Tuba,
    FrenchHorn,
    Guitar,
    Harp,
    Violin,
    Flute,
    Clarinet,
    PanFlute,
    NumInstruments
};

inline juce::String instrumentName (InstrumentType t)
{
    switch (t)
    {
        case InstrumentType::Harmonica:  return "Diatonic";
        case InstrumentType::Trumpet:    return "Trumpet";
        case InstrumentType::Tuba:       return "Tuba";
        case InstrumentType::FrenchHorn: return "Fr. Horn";
        case InstrumentType::Guitar:     return "Guitar";
        case InstrumentType::Harp:       return "Harp";
        case InstrumentType::Violin:     return "Violin";
        case InstrumentType::Flute:      return "Flute";
        case InstrumentType::Clarinet:   return "Clarinet";
        case InstrumentType::PanFlute:   return "Pan Flute";
        default: return "Unknown";
    }
}

} // namespace TurboTuning
