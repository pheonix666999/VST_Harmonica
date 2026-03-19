#pragma once
#include <JuceHeader.h>
#include "TurboLookAndFeel.h"
#include "../PluginProcessor.h"

namespace TurboTuning
{

// ─────────────────────────────────────────────────────────────────────────────
//  Forward declarations
// ─────────────────────────────────────────────────────────────────────────────
class BeatPadsPanel;
class HarpKeysPanel;
class MixerPanel;
class FxPanel;
class PluginStorePanel;

// ─────────────────────────────────────────────────────────────────────────────
//  DawEditor  –  Main GarageBand-style DAW UI
// ─────────────────────────────────────────────────────────────────────────────
class DawEditor : public juce::AudioProcessorEditor,
                  public juce::Timer
{
public:
    explicit DawEditor (TurboTuningProcessor& p);
    ~DawEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    juce::Image loadLogo() const;
    TurboTuningProcessor& proc;
    TurboLookAndFeel lnf;

    // ── Header ──────────────────────────────────────────────
    juce::Image  logoImage;
    juce::Label  titleLabel;
    juce::Label  subLabel;

    // Transport
    juce::TextButton playBtn   { "▶  PLAY" };
    juce::TextButton stopBtn   { "⏹  STOP" };
    juce::TextButton recBtn    { "⏺  REC" };
    juce::Slider     bpmSlider;
    juce::Label      bpmLabel;
    juce::Label      bpmValLabel;

    bool   isPlaying   = false;
    bool   isRecording = false;

    // ── Tab Bar ──────────────────────────────────────────────
    static constexpr int NUM_TABS = 4;
    juce::TextButton tabBtns[NUM_TABS];
    const char* TAB_LABELS[NUM_TABS] = { "EASY BEATS", "PLAY NOTES",
                                         "MIX", "FX" };
    int activeTab = 0;
    void switchTab (int t);

    // ── Tab Panels ———————————————————————————————————————————
    std::unique_ptr<BeatPadsPanel>    padsPanel;
    std::unique_ptr<HarpKeysPanel>    keysPanel;
    std::unique_ptr<MixerPanel>       mixerPanel;
    std::unique_ptr<FxPanel>          fxPanel;

    // ── Visualiser ───────────────────────────────────────────
    struct VisualizerState {
        float bars[32] = {};
        int   step = 0;
    } viz;

    // ── Status bar ───────────────────────────────────────────
    juce::Label statusLabel;
    void setStatus (const juce::String& msg);

    // Paint helpers
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DawEditor)
};

// ─────────────────────────────────────────────────────────────────────────────
//  BeatPadsPanel
// ─────────────────────────────────────────────────────────────────────────────
class BeatPadsPanel : public juce::Component, public juce::Timer
{
public:
    std::function<void(const juce::String&)> onStatus;

    BeatPadsPanel();
    ~BeatPadsPanel() override { stopTimer(); }
    void paint  (juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    struct Pad {
        juce::String label, emoji;
        juce::Colour colour;
        bool lit = false;
    };

    static constexpr int NPADS = 12;
    static constexpr int SEQ_ROWS = 4;
    static constexpr int SEQ_STEPS = 16;

    Pad pads[NPADS];
    juce::TextButton padBtns[NPADS];

    // Sequencer
    bool seqGrid[SEQ_ROWS][SEQ_STEPS] = {};
    juce::TextButton seqBtns[SEQ_ROWS][SEQ_STEPS];
    juce::TextButton seqPlayBtn { "▶  Play Loop" };
    juce::TextButton seqClearBtn { "Clear" };
    juce::Label seqRowLabels[SEQ_ROWS];
    bool seqRunning = false;
    int  seqStep    = 0;
    int  bpm        = 120;

    void triggerPad (int i);
    void buildPads();

    static const char* SEQ_ROW_NAMES[SEQ_ROWS];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BeatPadsPanel)
};

// ─────────────────────────────────────────────────────────────────────────────
//  HarpKeysPanel
// ─────────────────────────────────────────────────────────────────────────────
class HarpKeysPanel : public juce::Component, public juce::KeyListener
{
public:
    std::function<void(const juce::String&)> onStatus;

    explicit HarpKeysPanel (TurboTuningProcessor& p);
    ~HarpKeysPanel() override;
    void paint  (juce::Graphics& g) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress& kp, juce::Component* origin) override;

private:
    TurboTuningProcessor& proc;

    juce::ComboBox keySelect, octaveSelect, voiceSelect;
    juce::Label keyLbl, octLbl, voiceLbl;

    static constexpr int MAX_KEYS = 10;
    juce::TextButton keyBtns[MAX_KEYS];
    int numKeys = 0;

    // Chord buttons
    juce::TextButton chordBtns[4];
    juce::Label chordLabel;

    juce::StringArray scaleNotes;
    int octave = 4;

    void buildKeyButtons();
    void playNote (int index);

    static const juce::String SCALES[][10];
    static const int SCALE_SIZES[];
    static const char* SCALE_NAMES[];
    static const char* KEY_SHORTCUTS[];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HarpKeysPanel)
};

// ─────────────────────────────────────────────────────────────────────────────
//  MixerPanel
// ─────────────────────────────────────────────────────────────────────────────
class MixerPanel : public juce::Component
{
public:
    std::function<void(const juce::String&)> onStatus;

    explicit MixerPanel (TurboTuningProcessor& p);
    void paint  (juce::Graphics& g) override;
    void resized() override;

private:
    TurboTuningProcessor& proc;

    struct Track {
        juce::String name, emoji;
        juce::Slider fader;
        juce::TextButton muteBtn;
        juce::Label nameLabel;
        juce::Label volLabel;
        bool muted = false;
        float vol  = 80.f;
    };

    static constexpr int NTRACKS = 6;
    Track tracks[NTRACKS];

    juce::Slider masterFader;
    juce::Label  masterLabel, masterValLabel;

    void drawTrackBg (juce::Graphics& g, juce::Rectangle<int> r);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixerPanel)
};

// ─────────────────────────────────────────────────────────────────────────────
//  FxPanel – Effects
// ─────────────────────────────────────────────────────────────────────────────
class FxPanel : public juce::Component
{
public:
    std::function<void(const juce::String&)> onStatus;

    explicit FxPanel (TurboTuningProcessor& p);
    void paint  (juce::Graphics& g) override;
    void resized() override;

private:
    TurboTuningProcessor& proc;

    struct FxUnit {
        juce::String name, emoji;
        bool enabled = false;
        juce::Slider  knob;
        juce::TextButton toggle;
        juce::Label   nameLabel, valLabel;
    };

    static constexpr int NFX = 6;
    FxUnit fx[NFX];

    void initFx();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxPanel)
};

// ─────────────────────────────────────────────────────────────────────────────
//  PluginStorePanel
// ─────────────────────────────────────────────────────────────────────────────
class PluginStorePanel : public juce::Component, public juce::Timer
{
public:
    std::function<void(const juce::String&)> onStatus;

    PluginStorePanel();
    ~PluginStorePanel() override { stopTimer(); }
    void paint  (juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    struct Plugin {
        juce::String name, desc, emoji, category, size;
        bool    installed    = false;
        bool    downloading  = false;
        float   dlProgress   = 0.f;
    };

    juce::TextEditor searchBox;
    juce::TextButton catBtns[5];
    static const char* CATS[5];
    int activeCat = 0;

    juce::OwnedArray<juce::TextButton> dlBtns;
    juce::Viewport  viewport;
    juce::Component listContainer;

    std::vector<Plugin> plugins;
    std::vector<int>    filteredIdx;

    void buildPlugins();
    void filterPlugins (int cat, const juce::String& search);
    void rebuildList();
    void startDownload (int pluginIdx);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginStorePanel)
};

} // namespace TurboTuning
