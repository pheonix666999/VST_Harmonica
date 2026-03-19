#include "DawEditor.h"

namespace TurboTuning
{

// ─────────────────────────────────────────────────
//  Colour helpers
// ─────────────────────────────────────────────────
static juce::Colour col (juce::uint32 argb) { return juce::Colour (argb); }

static const juce::Colour BG     = col (TurboLookAndFeel::bgBody);
static const juce::Colour BG2    = col (TurboLookAndFeel::bgApp);
static const juce::Colour CARD   = col (TurboLookAndFeel::bgCard);
static const juce::Colour GOLD   = col (TurboLookAndFeel::gold);
static const juce::Colour BLUE   = col (TurboLookAndFeel::blue);
static const juce::Colour GREEN  = col (TurboLookAndFeel::green);
static const juce::Colour RED    = col (TurboLookAndFeel::red);
static const juce::Colour TEXT   = col (TurboLookAndFeel::textPrimary);
static const juce::Colour TDIM   = col (TurboLookAndFeel::textDim);
static const juce::Colour BORDER = col (TurboLookAndFeel::border);

static void drawCard (juce::Graphics& g, juce::Rectangle<int> r, float radius = 10.f)
{
    g.setColour (col (TurboLookAndFeel::bgPanel));
    g.fillRoundedRectangle (r.toFloat(), radius);
    g.setColour (BORDER);
    g.drawRoundedRectangle (r.toFloat().reduced (0.5f), radius, 1.f);
}

juce::Image DawEditor::loadLogo() const
{
    const auto exeFile = juce::File::getSpecialLocation (juce::File::currentExecutableFile);
    const juce::File candidates[] = {
        exeFile.getSiblingFile ("logo.png"),
        exeFile.getParentDirectory().getSiblingFile ("demo").getChildFile ("logo.png"),
        exeFile.getParentDirectory().getParentDirectory().getSiblingFile ("demo").getChildFile ("logo.png")
    };

    for (const auto& file : candidates)
        if (file.existsAsFile())
            return juce::ImageFileFormat::loadFrom (file);

    return {};
}

// ─────────────────────────────────────────────────
//  DawEditor
// ─────────────────────────────────────────────────
DawEditor::DawEditor (TurboTuningProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    setOpaque (true);
    setSize (1120, 760);

    logoImage = loadLogo();

    titleLabel.setText ("TURBO HARP", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (26.f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, GOLD);
    addAndMakeVisible (titleLabel);

    subLabel.setText ("GarageBand, but very easy to use.", juce::dontSendNotification);
    subLabel.setFont (juce::Font (12.f));
    subLabel.setColour (juce::Label::textColourId, TDIM);
    addAndMakeVisible (subLabel);

    // Transport buttons
    playBtn.setColour (juce::TextButton::buttonColourId, col (TurboLookAndFeel::bgCard));
    playBtn.setColour (juce::TextButton::textColourOffId, GREEN);
    playBtn.onClick = [this] {
        isPlaying = !isPlaying;
        playBtn.setButtonText (isPlaying ? "⏸  PAUSE" : "▶  PLAY");
        setStatus (isPlaying ? "▶  Playing..." : "⏸  Paused");
    };
    addAndMakeVisible (playBtn);

    stopBtn.setColour (juce::TextButton::buttonColourId, col (TurboLookAndFeel::bgCard));
    stopBtn.setColour (juce::TextButton::textColourOffId, TEXT);
    stopBtn.onClick = [this] {
        isPlaying = false; isRecording = false;
        playBtn.setButtonText ("▶  PLAY");
        recBtn.setColour (juce::TextButton::textColourOffId, RED);
        setStatus ("⏹  Stopped");
    };
    addAndMakeVisible (stopBtn);

    recBtn.setColour (juce::TextButton::buttonColourId, col (TurboLookAndFeel::bgCard));
    recBtn.setColour (juce::TextButton::textColourOffId, RED);
    recBtn.onClick = [this] {
        isRecording = !isRecording;
        recBtn.setColour (juce::TextButton::textColourOffId,
                          isRecording ? juce::Colours::white : RED);
        setStatus (isRecording ? "⏺  Recording! Play something..." : "⏹  Recording stopped");
    };
    addAndMakeVisible (recBtn);

    // BPM
    bpmLabel.setText ("BPM", juce::dontSendNotification);
    bpmLabel.setFont (juce::Font (10.f, juce::Font::bold));
    bpmLabel.setColour (juce::Label::textColourId, TDIM);
    addAndMakeVisible (bpmLabel);

    bpmValLabel.setText ("120", juce::dontSendNotification);
    bpmValLabel.setFont (juce::Font (18.f, juce::Font::bold));
    bpmValLabel.setColour (juce::Label::textColourId, GOLD);
    addAndMakeVisible (bpmValLabel);

    bpmSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    bpmSlider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    bpmSlider.setRange (40, 240, 1);
    bpmSlider.setValue (120);
    bpmSlider.onValueChange = [this] {
        bpmValLabel.setText (juce::String ((int) bpmSlider.getValue()), juce::dontSendNotification);
    };
    addAndMakeVisible (bpmSlider);

    // Tabs
    for (int i = 0; i < NUM_TABS; ++i)
    {
        tabBtns[i].setButtonText (TAB_LABELS[i]);
        tabBtns[i].setColour (juce::TextButton::buttonColourId, col (TurboLookAndFeel::bgApp));
        tabBtns[i].setColour (juce::TextButton::textColourOffId, TDIM);
        tabBtns[i].setColour (juce::TextButton::textColourOnId, GOLD);
        int idx = i;
        tabBtns[i].onClick = [this, idx] { switchTab (idx); };
        addAndMakeVisible (tabBtns[i]);
    }

    // Sub-panels
    padsPanel   = std::make_unique<BeatPadsPanel>();
    keysPanel   = std::make_unique<HarpKeysPanel>(proc);
    mixerPanel  = std::make_unique<MixerPanel>(proc);
    fxPanel     = std::make_unique<FxPanel>(proc);
    padsPanel->onStatus   = [this](auto m){ setStatus(m); };
    keysPanel->onStatus   = [this](auto m){ setStatus(m); };
    mixerPanel->onStatus  = [this](auto m){ setStatus(m); };
    fxPanel->onStatus     = [this](auto m){ setStatus(m); };

    addAndMakeVisible (*padsPanel);
    addChildComponent (*keysPanel);
    addChildComponent (*mixerPanel);
    addChildComponent (*fxPanel);

    // Status
    statusLabel.setText ("Choose a tab and start playing.", juce::dontSendNotification);
    statusLabel.setFont (juce::Font (11.f));
    statusLabel.setColour (juce::Label::textColourId, TDIM);
    statusLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (statusLabel);

    switchTab (0);
    startTimerHz (30);
}

DawEditor::~DawEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void DawEditor::setStatus (const juce::String& msg)
{
    statusLabel.setText (msg, juce::dontSendNotification);
}

void DawEditor::switchTab (int t)
{
    activeTab = t;
    for (int i = 0; i < NUM_TABS; ++i)
        tabBtns[i].setToggleState (i == t, juce::dontSendNotification);

    padsPanel->setVisible   (t == 0);
    keysPanel->setVisible   (t == 1);
    mixerPanel->setVisible  (t == 2);
    fxPanel->setVisible     (t == 3);
}

void DawEditor::timerCallback()
{
    // Drive idle visualiser animation
    for (auto& b : viz.bars)
    {
        b += (juce::Random::getSystemRandom().nextFloat() * 0.15f - 0.075f);
        b = juce::jlimit (0.05f, 0.95f, b);
        b *= isPlaying ? 1.0f : 0.97f;
    }
    repaint (0, getHeight() - 88, getWidth(), 30);
}

// ─── paint ────────────────────────────────────────
void DawEditor::paint (juce::Graphics& g)
{
    g.fillAll (BG);

    // header gradient
    auto hdr = getLocalBounds().removeFromTop (80);
    juce::ColourGradient hdrGrad (BG2, 0, 0, BG, 0, 80, false);
    g.setGradientFill (hdrGrad);
    g.fillRect (hdr);
    g.setColour (BORDER);
    g.drawLine (0, 80, (float)getWidth(), 80, 1.f);

    // tab bar
    g.setColour (col (TurboLookAndFeel::bgApp));
    g.fillRect (0, 80, getWidth(), 44);
    g.setColour (BORDER);
    g.drawLine (0, 124, (float)getWidth(), 124, 1.f);

    // active tab indicator
    int tabW = getWidth() / NUM_TABS;
    g.setColour (GOLD);
    g.fillRect (activeTab * tabW, 120, tabW, 4);

    // visualiser area
    auto vizRect = getLocalBounds().removeFromBottom (58);
    g.setColour (col (TurboLookAndFeel::bgApp));
    g.fillRect (vizRect);
    g.setColour (BORDER);
    g.drawLine (0, (float)vizRect.getY(), (float)getWidth(), (float)vizRect.getY(), 1.f);

    // draw viz bars
    int nb = (int)std::size (viz.bars);
    float bw = (float)getWidth() / nb;
    for (int i = 0; i < nb; ++i)
    {
        float h = viz.bars[i] * 26.f;
        float hue = 200.f + (float)i / nb * 120.f;
        g.setColour (juce::Colour::fromHSV (hue / 360.f, 0.8f, 0.85f, 1.f));
        float bx = i * bw + 1.f;
        float by = (float)vizRect.getY() + 4.f + (26.f - h);
        g.fillRoundedRectangle (bx, by, bw - 2.f, h, 2.f);
    }

    // status bar
    g.setColour (BG);
    g.fillRect (0, vizRect.getY() + 30, getWidth(), 28);
    g.setColour (BORDER);
    g.drawLine (0, (float)vizRect.getY() + 30.f, (float)getWidth(), (float)vizRect.getY() + 30.f, 1.f);

    if (logoImage.isValid())
    {
        auto logoBounds = juce::Rectangle<float> (14.0f, 14.0f, 52.0f, 52.0f);
        g.drawImageWithin (logoImage,
                           (int) logoBounds.getX(), (int) logoBounds.getY(),
                           (int) logoBounds.getWidth(), (int) logoBounds.getHeight(),
                           juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize,
                           false);
    }
    else
    {
        g.setColour (GOLD.withAlpha (0.15f));
        g.fillRoundedRectangle (12, 14, 52, 52, 10.f);
        g.setColour (GOLD);
        g.setFont (26.f);
        g.drawText (juce::String::charToString (0x1F3B5), 12, 14, 52, 52, juce::Justification::centred);
    }
}

// ─── resized ──────────────────────────────────────
void DawEditor::resized()
{
    auto area = getLocalBounds();

    // Header 80px
    auto hdr = area.removeFromTop (80);

    // Logo
    int logoX = 12;
    if (logoImage.isValid())
    {
        // drawn in paint
    }

    // Title block
    auto titleBlock = hdr.removeFromLeft (340).withTrimmedLeft (72);
    titleLabel.setBounds (titleBlock.removeFromTop (44));
    subLabel.setBounds   (titleBlock);

    // Transport (right side)
    auto transport = hdr;
    bpmValLabel.setBounds (transport.removeFromLeft (50).reduced (4, 20));
    bpmLabel.setBounds    (transport.removeFromLeft (36).reduced (4, 30));
    bpmSlider.setBounds   (transport.removeFromLeft (110).reduced (0, 28));
    transport.removeFromLeft (10);
    playBtn.setBounds (transport.removeFromLeft (100).reduced (2, 18));
    stopBtn.setBounds (transport.removeFromLeft (90).reduced (2, 18));
    recBtn.setBounds  (transport.removeFromLeft (90).reduced (2, 18));

    // Tab bar 44px
    auto tabBar = area.removeFromTop (44);
    int tw = tabBar.getWidth() / NUM_TABS;
    for (int i = 0; i < NUM_TABS; ++i)
        tabBtns[i].setBounds (tabBar.removeFromLeft (tw));

    // Bottom area
    auto bottom = area.removeFromBottom (58);
    statusLabel.setBounds (bottom.removeFromBottom (28).reduced (12, 4));
    // viz uses remaining bottom

    // Content
    auto content = area.reduced (12, 8);
    padsPanel->setBounds   (content);
    keysPanel->setBounds   (content);
    mixerPanel->setBounds  (content);
    fxPanel->setBounds     (content);
}

// ─────────────────────────────────────────────────
//  BeatPadsPanel
// ─────────────────────────────────────────────────
const char* BeatPadsPanel::SEQ_ROW_NAMES[SEQ_ROWS] = { "Kick", "Snare", "Hi-Hat", "Clap" };

BeatPadsPanel::BeatPadsPanel()
{
    struct PadDef { const char* label; const char* emoji; juce::uint32 colour; };
    static const PadDef DEFS[NPADS] = {
        {"Kick",    "\xf0\x9f\xa5\x81", 0xFF6C63FF},
        {"Snare",   "\xf0\x9f\x92\xa5", 0xFFA78BFA},
        {"Hi-Hat",  "\xf0\x9f\x8e\xa9", 0xFF38BDF8},
        {"Clap",    "\xf0\x9f\x91\x8f", 0xFF22D3A5},
        {"Bass",    "\xf0\x9f\x8e\xb8", 0xFFF59E0B},
        {"Synth",   "\xf0\x9f\x8e\xb9", 0xFFEC4899},
        {"Cowbell", "\xf0\x9f\x94\x94", 0xFF84CC16},
        {"Tom",     "\xf0\x9f\xa5\x81", 0xFFF97316},
        {"Open HH", "\xf0\x9f\x94\x8a", 0xFF06B6D4},
        {"Rim",     "\xf0\x9f\xaa\x98", 0xFF8B5CF6},
        {"Perc",    "\xf0\x9f\x8e\xb6", 0xFFD946EF},
        {"FX",      "\xe2\x9c\xa8",       0xFFE11D48},
    };

    for (int i = 0; i < NPADS; ++i)
    {
        pads[i] = { DEFS[i].label, DEFS[i].emoji, juce::Colour (DEFS[i].colour) };
        padBtns[i].setButtonText (juce::String (DEFS[i].emoji) + "\n" + DEFS[i].label);
        padBtns[i].setColour (juce::TextButton::buttonColourId, juce::Colour (DEFS[i].colour).withAlpha (0.6f));
        padBtns[i].setColour (juce::TextButton::textColourOffId, juce::Colours::white);
        int idx = i;
        padBtns[i].onClick = [this, idx] { triggerPad (idx); };
        addAndMakeVisible (padBtns[i]);
    }

    // Sequencer grid
    for (int r = 0; r < SEQ_ROWS; ++r)
    {
        seqRowLabels[r].setText (SEQ_ROW_NAMES[r], juce::dontSendNotification);
        seqRowLabels[r].setFont (juce::Font (10.f, juce::Font::bold));
        seqRowLabels[r].setColour (juce::Label::textColourId, col (TurboLookAndFeel::textDim));
        seqRowLabels[r].setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (seqRowLabels[r]);

        for (int s = 0; s < SEQ_STEPS; ++s)
        {
            seqBtns[r][s].setColour (juce::TextButton::buttonColourId, col (TurboLookAndFeel::bgInput));
            seqBtns[r][s].setColour (juce::TextButton::buttonOnColourId, col (TurboLookAndFeel::gold));
            seqBtns[r][s].setClickingTogglesState (true);
            int rr = r, ss = s;
            seqBtns[r][s].onClick = [this, rr, ss] { seqGrid[rr][ss] = seqBtns[rr][ss].getToggleState(); };
            addAndMakeVisible (seqBtns[r][s]);
        }
    }

    seqPlayBtn.setColour (juce::TextButton::buttonColourId, col (TurboLookAndFeel::bgCard));
    seqPlayBtn.setColour (juce::TextButton::textColourOffId, GREEN);
    seqPlayBtn.onClick = [this] {
        seqRunning = !seqRunning;
        seqPlayBtn.setButtonText (seqRunning ? "⏹  Stop Loop" : "▶  Play Loop");
        if (seqRunning) startTimerHz ((bpm * 4) / 60);
        else            stopTimer();
    };
    addAndMakeVisible (seqPlayBtn);

    seqClearBtn.onClick = [this] {
        for (auto& row : seqGrid) for (auto& b : row) b = false;
        for (auto& row : seqBtns) for (auto& b : row) b.setToggleState (false, juce::dontSendNotification);
    };
    addAndMakeVisible (seqClearBtn);
}

void BeatPadsPanel::triggerPad (int i)
{
    pads[i].lit = true;
    juce::Timer::callAfterDelay (120, [this, i] { pads[i].lit = false; repaint(); });
    repaint();
    if (onStatus) onStatus ("\xf0\x9f\xa5\x81 " + pads[i].label + "!");
}

void BeatPadsPanel::timerCallback()
{
    // advance sequencer step
    for (int r = 0; r < SEQ_ROWS; ++r)
        if (seqGrid[r][seqStep]) triggerPad (r);
    seqStep = (seqStep + 1) % SEQ_STEPS;
    repaint();
}

void BeatPadsPanel::paint (juce::Graphics& g)
{
    // Highlight active seq step column
    if (seqRunning)
    {
        auto bounds = getLocalBounds();
        bounds.removeFromTop (bounds.getHeight() * 55 / 100);
        bounds.removeFromTop (36);
        int stepW = (bounds.getWidth() - 52) / SEQ_STEPS;
        g.setColour (GOLD.withAlpha (0.08f));
        g.fillRect ((52 + seqStep * stepW), bounds.getY(), stepW, bounds.getHeight());
    }
}

void BeatPadsPanel::resized()
{
    auto area = getLocalBounds();

    // Pad grid (top ~55%)
    auto padArea = area.removeFromTop (area.getHeight() * 55 / 100);
    int cols = 4, rows = 3;
    int pw = padArea.getWidth() / cols, ph = padArea.getHeight() / rows;
    for (int i = 0; i < NPADS; ++i)
        padBtns[i].setBounds ((i % cols) * pw, (i / cols) * ph, pw - 6, ph - 6);

    // Sequencer controls row
    auto ctrlRow = area.removeFromTop (36);
    seqPlayBtn.setBounds  (ctrlRow.removeFromLeft (130).reduced (2, 4));
    seqClearBtn.setBounds (ctrlRow.removeFromLeft (80).reduced (2, 4));

    // Sequencer grid
    int labelW = 52;
    int stepW  = (area.getWidth() - labelW) / SEQ_STEPS;
    int rowH   = area.getHeight() / SEQ_ROWS;

    for (int r = 0; r < SEQ_ROWS; ++r)
    {
        auto row = area.removeFromTop (rowH).reduced (0, 2);
        seqRowLabels[r].setBounds (row.removeFromLeft (labelW));
        for (int s = 0; s < SEQ_STEPS; ++s)
            seqBtns[r][s].setBounds (row.removeFromLeft (stepW).reduced (1));
    }
}

// ─────────────────────────────────────────────────
//  HarpKeysPanel
// ─────────────────────────────────────────────────
const juce::String HarpKeysPanel::SCALES[][10] = {
    { "C","D","E","F","G","A","B","","","" },      // C Major (7)
    { "G","A","B","C","D","E","F#","","","" },     // G Major (7)
    { "F","G","A","Bb","C","D","E","","","" },     // F Major (7)
    { "C","Eb","F","F#","G","Bb","","","","" },    // C Blues (6)
    { "C","D","E","G","A","","","","","" },         // C Pentatonic (5)
};
const int HarpKeysPanel::SCALE_SIZES[]  = { 7, 7, 7, 6, 5 };
const char* HarpKeysPanel::SCALE_NAMES[] = { "C Major", "G Major", "F Major", "C Blues", "C Pentatonic" };
const char* HarpKeysPanel::KEY_SHORTCUTS[] = { "A","S","D","F","G","H","J","K","L",";" };

HarpKeysPanel::HarpKeysPanel (TurboTuningProcessor& p) : proc (p)
{
    // Key select
    keyLbl.setText ("Scale:", juce::dontSendNotification);
    keyLbl.setFont (juce::Font (11.f, juce::Font::bold));
    keyLbl.setColour (juce::Label::textColourId, TDIM);
    addAndMakeVisible (keyLbl);

    for (int i = 0; i < 5; ++i)
        keySelect.addItem (SCALE_NAMES[i], i + 1);
    keySelect.setSelectedId (1, juce::dontSendNotification);
    keySelect.onChange = [this] { buildKeyButtons(); };
    addAndMakeVisible (keySelect);

    octLbl.setText ("Octave:", juce::dontSendNotification);
    octLbl.setFont (juce::Font (11.f, juce::Font::bold));
    octLbl.setColour (juce::Label::textColourId, TDIM);
    addAndMakeVisible (octLbl);

    for (int i = 2; i <= 6; ++i)
        octaveSelect.addItem (juce::String (i), i);
    octaveSelect.setSelectedId (4, juce::dontSendNotification);
    octaveSelect.onChange = [this] { octave = octaveSelect.getSelectedId(); };
    addAndMakeVisible (octaveSelect);

    voiceLbl.setText ("Sound:", juce::dontSendNotification);
    voiceLbl.setFont (juce::Font (11.f, juce::Font::bold));
    voiceLbl.setColour (juce::Label::textColourId, TDIM);
    addAndMakeVisible (voiceLbl);

    voiceSelect.addItem ("Harmonica", 1);
    voiceSelect.addItem ("Piano",     2);
    voiceSelect.addItem ("Synth",     3);
    voiceSelect.addItem ("Organ",     4);
    voiceSelect.setSelectedId (1, juce::dontSendNotification);
    addAndMakeVisible (voiceSelect);

    for (int i = 0; i < MAX_KEYS; ++i)
    {
        keyBtns[i].setColour (juce::TextButton::buttonColourId, col (TurboLookAndFeel::bgCard));
        keyBtns[i].setColour (juce::TextButton::textColourOffId, TEXT);
        int idx = i;
        keyBtns[i].onClick = [this, idx] { playNote (idx); };
        addAndMakeVisible (keyBtns[i]);
    }

    // Chord strip
    chordLabel.setText ("Quick Chords:", juce::dontSendNotification);
    chordLabel.setFont (juce::Font (11.f, juce::Font::bold));
    chordLabel.setColour (juce::Label::textColourId, TDIM);
    addAndMakeVisible (chordLabel);

    const char* chordNames[] = { "I", "IV", "V", "vi" };
    for (int i = 0; i < 4; ++i)
    {
        chordBtns[i].setButtonText (chordNames[i]);
        chordBtns[i].setColour (juce::TextButton::buttonColourId, col (TurboLookAndFeel::bgCard));
        chordBtns[i].setColour (juce::TextButton::textColourOffId, GOLD);
        int ci = i;
        chordBtns[i].onClick = [this, ci]
        {
            // Play simple chord arpeggio
            int offsets[][3] = {{0,2,4},{3,5,7},{4,6,9},{5,7,9}};
            auto& sc = SCALES[keySelect.getSelectedId() - 1];
            int sz = SCALE_SIZES[keySelect.getSelectedId() - 1];
            for (int j = 0; j < 3; ++j)
            {
                int ni = offsets[ci][j] % sz;
                juce::Timer::callAfterDelay (j * 80, [this, ni] { playNote (ni); });
            }
        };
        addAndMakeVisible (chordBtns[i]);
    }

    buildKeyButtons();
    addKeyListener (this);
}

HarpKeysPanel::~HarpKeysPanel()
{
    removeKeyListener (this);
}

void HarpKeysPanel::buildKeyButtons()
{
    int scaleIdx = keySelect.getSelectedId() - 1;
    numKeys = SCALE_SIZES[scaleIdx];
    for (int i = 0; i < MAX_KEYS; ++i)
    {
        if (i < numKeys)
        {
            keyBtns[i].setButtonText (SCALES[scaleIdx][i] + "\n" + KEY_SHORTCUTS[i]);
            keyBtns[i].setVisible (true);
        }
        else keyBtns[i].setVisible (false);
    }
    resized();
}

void HarpKeysPanel::playNote (int index)
{
    if (index >= numKeys) return;
    int scaleIdx = keySelect.getSelectedId() - 1;
    juce::String note = SCALES[scaleIdx][index];
    proc.setKey (note.substring (0, 1).toUpperCase() + note.substring (1));
    proc.playHole (index + 1, true);
    juce::Timer::callAfterDelay (400, [this, index] { proc.stopHole (index + 1, true); });
    keyBtns[index].setColour (juce::TextButton::buttonColourId, GOLD.withAlpha (0.4f));
    juce::Timer::callAfterDelay (150, [this, index] {
        keyBtns[index].setColour (juce::TextButton::buttonColourId, col (TurboLookAndFeel::bgCard));
    });
    if (onStatus) onStatus ("\xf0\x9f\x8e\xb5 Playing " + SCALES[scaleIdx][index]);
}

bool HarpKeysPanel::keyPressed (const juce::KeyPress& kp, juce::Component*)
{
    const char* shortcuts = "asdfghjkl;";
    for (int i = 0; shortcuts[i]; ++i)
        if (kp == juce::KeyPress (shortcuts[i])) { playNote (i); return true; }
    return false;
}

void HarpKeysPanel::paint (juce::Graphics&) {}

void HarpKeysPanel::resized()
{
    auto area = getLocalBounds();

    // Controls row
    auto ctrlRow = area.removeFromTop (40);
    keyLbl.setBounds (ctrlRow.removeFromLeft (44).reduced (2, 8));
    keySelect.setBounds (ctrlRow.removeFromLeft (140).reduced (2, 6));
    octLbl.setBounds (ctrlRow.removeFromLeft (52).reduced (2, 8));
    octaveSelect.setBounds (ctrlRow.removeFromLeft (80).reduced (2, 6));
    voiceLbl.setBounds (ctrlRow.removeFromLeft (50).reduced (2, 8));
    voiceSelect.setBounds (ctrlRow.removeFromLeft (120).reduced (2, 6));

    // Key buttons
    auto keyArea = area.removeFromTop (area.getHeight() - 60);
    if (numKeys > 0)
    {
        int kw = keyArea.getWidth() / numKeys;
        for (int i = 0; i < numKeys; ++i)
            keyBtns[i].setBounds (i * kw, keyArea.getY(), kw - 8, keyArea.getHeight());
    }

    // Chord strip
    auto chordRow = area;
    chordLabel.setBounds (chordRow.removeFromLeft (100).reduced (2, 8));
    int cw = chordRow.getWidth() / 4;
    for (auto& b : chordBtns)
        b.setBounds (chordRow.removeFromLeft (cw).reduced (4, 4));
}

// ─────────────────────────────────────────────────
//  MixerPanel
// ─────────────────────────────────────────────────
static const struct { const char* name; const char* emoji; } TRACK_DEFS[] = {
    {"Harp","🎵"},{"Drums","🥁"},{"Bass","🎸"},{"Synth","🎹"},{"FX","✨"},{"Vox","🎤"}
};

MixerPanel::MixerPanel (TurboTuningProcessor& p) : proc (p)
{
    for (int i = 0; i < NTRACKS; ++i)
    {
        tracks[i].name  = TRACK_DEFS[i].name;
        tracks[i].emoji = TRACK_DEFS[i].emoji;
        tracks[i].vol   = 80.f;

        tracks[i].nameLabel.setText (tracks[i].emoji + " " + tracks[i].name, juce::dontSendNotification);
        tracks[i].nameLabel.setFont (juce::Font (11.f, juce::Font::bold));
        tracks[i].nameLabel.setJustificationType (juce::Justification::centred);
        tracks[i].nameLabel.setColour (juce::Label::textColourId, TEXT);
        addAndMakeVisible (tracks[i].nameLabel);

        tracks[i].fader.setSliderStyle (juce::Slider::LinearVertical);
        tracks[i].fader.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        tracks[i].fader.setRange (0, 100, 1);
        tracks[i].fader.setValue (80);
        int ti = i;
        tracks[i].fader.onValueChange = [this, ti] {
            tracks[ti].vol = (float)tracks[ti].fader.getValue();
            tracks[ti].volLabel.setText (juce::String ((int)tracks[ti].vol) + "%", juce::dontSendNotification);
            if (ti == 0) proc.setMasterVolume (tracks[0].vol / 100.f);
        };
        addAndMakeVisible (tracks[i].fader);

        tracks[i].volLabel.setText ("80%", juce::dontSendNotification);
        tracks[i].volLabel.setFont (juce::Font (11.f));
        tracks[i].volLabel.setColour (juce::Label::textColourId, GOLD);
        tracks[i].volLabel.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (tracks[i].volLabel);

        tracks[i].muteBtn.setButtonText ("M");
        tracks[i].muteBtn.setColour (juce::TextButton::buttonColourId, col (TurboLookAndFeel::bgCard));
        tracks[i].muteBtn.setColour (juce::TextButton::textColourOffId, TDIM);
        tracks[i].muteBtn.onClick = [this, ti] {
            tracks[ti].muted = !tracks[ti].muted;
            tracks[ti].muteBtn.setColour (juce::TextButton::buttonColourId,
                tracks[ti].muted ? RED : col (TurboLookAndFeel::bgCard));
            tracks[ti].muteBtn.setColour (juce::TextButton::textColourOffId,
                tracks[ti].muted ? juce::Colours::white : TDIM);
        };
        addAndMakeVisible (tracks[i].muteBtn);
    }

    masterLabel.setText ("MASTER", juce::dontSendNotification);
    masterLabel.setFont (juce::Font (12.f, juce::Font::bold));
    masterLabel.setColour (juce::Label::textColourId, GOLD);
    masterLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (masterLabel);

    masterFader.setSliderStyle (juce::Slider::LinearHorizontal);
    masterFader.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    masterFader.setRange (0, 100, 1);
    masterFader.setValue (80);
    masterFader.onValueChange = [this] {
        proc.setMasterVolume ((float)masterFader.getValue() / 100.f);
        masterValLabel.setText (juce::String ((int)masterFader.getValue()) + "%",
                                juce::dontSendNotification);
    };
    addAndMakeVisible (masterFader);

    masterValLabel.setText ("80%", juce::dontSendNotification);
    masterValLabel.setFont (juce::Font (13.f, juce::Font::bold));
    masterValLabel.setColour (juce::Label::textColourId, GOLD);
    addAndMakeVisible (masterValLabel);
}

void MixerPanel::paint (juce::Graphics& g)
{
    for (int i = 0; i < NTRACKS; ++i)
    {
        auto r = tracks[i].fader.getBounds().expanded (10, 20);
        drawCard (g, r);
    }
    auto masterRow = masterFader.getBounds().expanded (10, 10);
    drawCard (g, masterRow.withLeft (masterRow.getX() - 80).withRight (getWidth() - 4));
}

void MixerPanel::resized()
{
    auto area = getLocalBounds().reduced (4);
    auto masterRow = area.removeFromBottom (52);
    masterLabel.setBounds (masterRow.removeFromLeft (80).reduced (4, 10));
    masterValLabel.setBounds (masterRow.removeFromRight (50).reduced (4, 10));
    masterFader.setBounds (masterRow.reduced (4, 10));

    int tw = area.getWidth() / NTRACKS;
    for (int i = 0; i < NTRACKS; ++i)
    {
        auto col2 = area.removeFromLeft (tw).reduced (8, 4);
        tracks[i].nameLabel.setBounds (col2.removeFromTop (24));
        tracks[i].muteBtn.setBounds (col2.removeFromBottom (24).reduced (8, 0));
        tracks[i].volLabel.setBounds (col2.removeFromBottom (20));
        tracks[i].fader.setBounds (col2);
    }
}

// ─────────────────────────────────────────────────
//  FxPanel
// ─────────────────────────────────────────────────
void FxPanel::initFx()
{
    struct Def { const char* name; const char* emoji; bool on; };
    static const Def DEFS[NFX] = {
        {"Reverb",     "\xf0\x9f\x8f\x9f", true},
        {"Delay",      "\xf0\x9f\x94\x81", false},
        {"Distortion", "\xf0\x9f\x94\xa5", false},
        {"Chorus",     "\xf0\x9f\x8c\x8a", false},
        {"Auto-Tune",  "\xf0\x9f\x8e\xaf", true},
        {"Safe Mode",  "\xf0\x9f\x9b\xa1",  true},
    };

    for (int i = 0; i < NFX; ++i)
    {
        fx[i].name    = DEFS[i].name;
        fx[i].emoji   = DEFS[i].emoji;
        fx[i].enabled = DEFS[i].on;

        fx[i].nameLabel.setText (juce::String (DEFS[i].emoji) + "  " + DEFS[i].name,
                                 juce::dontSendNotification);
        fx[i].nameLabel.setFont (juce::Font (13.f, juce::Font::bold));
        fx[i].nameLabel.setColour (juce::Label::textColourId, TEXT);
        addAndMakeVisible (fx[i].nameLabel);

        fx[i].knob.setSliderStyle (juce::Slider::LinearHorizontal);
        fx[i].knob.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        fx[i].knob.setRange (0, 100, 1);
        fx[i].knob.setValue (50);
        int fi = i;
        fx[i].knob.onValueChange = [this, fi] {
            int val = (int)fx[fi].knob.getValue();
            fx[fi].valLabel.setText (juce::String (val) + "%", juce::dontSendNotification);
            switch (fi) {
                case 0: proc.setReverbLevel (val / 100.f); break;
                case 1: proc.setDelayLevel  (val / 100.f); break;
                case 2: proc.setChorusLevel (val / 100.f); break;
                default: break;
            }
        };
        addAndMakeVisible (fx[i].knob);

        fx[i].valLabel.setText ("50%", juce::dontSendNotification);
        fx[i].valLabel.setFont (juce::Font (11.f));
        fx[i].valLabel.setColour (juce::Label::textColourId, GOLD);
        fx[i].valLabel.setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (fx[i].valLabel);

        fx[i].toggle.setButtonText (DEFS[i].on ? "ON" : "OFF");
        fx[i].toggle.setColour (juce::TextButton::buttonColourId,
                                DEFS[i].on ? GREEN.withAlpha(0.8f) : col(TurboLookAndFeel::bgCard));
        fx[i].toggle.setColour (juce::TextButton::textColourOffId,
                                DEFS[i].on ? juce::Colours::black : TDIM);
        int ii = i;
        fx[i].toggle.onClick = [this, ii] {
            fx[ii].enabled = !fx[ii].enabled;
            bool on = fx[ii].enabled;
            fx[ii].toggle.setButtonText (on ? "ON" : "OFF");
            fx[ii].toggle.setColour (juce::TextButton::buttonColourId,
                                     on ? GREEN.withAlpha(0.8f) : col(TurboLookAndFeel::bgCard));
            fx[ii].toggle.setColour (juce::TextButton::textColourOffId,
                                     on ? juce::Colours::black : TDIM);
            if (onStatus) onStatus (fx[ii].name + (on ? " ON" : " OFF"));
            switch (ii) {
                case 4: proc.setAutoTuneEnabled (on);  break;
                case 5: proc.setSafeModeEnabled (on);  break;
                default: break;
            }
        };
        addAndMakeVisible (fx[i].toggle);
    }
}

FxPanel::FxPanel (TurboTuningProcessor& p) : proc (p) { initFx(); }

void FxPanel::paint (juce::Graphics& g)
{
    int rowH = getHeight() / NFX;
    for (int i = 0; i < NFX; ++i)
        drawCard (g, getLocalBounds().withY (i * rowH).withHeight (rowH - 4).reduced (2, 0));
}

void FxPanel::resized()
{
    int rowH = getHeight() / NFX;
    for (int i = 0; i < NFX; ++i)
    {
        auto row = getLocalBounds().withY (i * rowH).withHeight (rowH - 4).reduced (8, 4);
        fx[i].nameLabel.setBounds (row.removeFromLeft (180));
        fx[i].toggle.setBounds    (row.removeFromRight (60).reduced (2, 2));
        fx[i].valLabel.setBounds  (row.removeFromRight (50));
        fx[i].knob.setBounds      (row);
    }
}

// ─────────────────────────────────────────────────
//  PluginStorePanel
// ─────────────────────────────────────────────────
const char* PluginStorePanel::CATS[5] = { "All", "Instruments", "Effects", "Loops", "Drums" };

PluginStorePanel::PluginStorePanel()
{
    plugins = {
        {"Jazz Harmonica Pro",  "Pro jazz harp with breath control",       "\xf0\x9f\x8e\xb5","Instruments","12 MB"},
        {"Deep Bass Synth",     "Punchy 808 sub for trap & hip-hop",       "\xf0\x9f\x8e\xb8","Instruments","8 MB"},
        {"Trap Drum Kit",       "808s, claps, hi-hats – all preloaded",    "\xf0\x9f\xa5\x81","Drums","45 MB"},
        {"Lo-Fi Piano",         "Warm vintage piano with tape saturation", "\xf0\x9f\x8e\xb9","Instruments","30 MB"},
        {"Dreamy Reverb",       "Lush spring reverb – sounds huge",        "\xf0\x9f\x8f\x9f","Effects","2 MB"},
        {"Vintage Tape Delay",  "Classic tape echo with wow & flutter",    "\xf0\x9f\x93\xbc","Effects","3 MB"},
        {"Chill Lo-Fi Loops",   "100+ lo-fi loops in all keys",            "\xf0\x9f\x8c\x99","Loops","120 MB"},
        {"EDM Drop Kit",        "Risers, drops, FX for your next banger",  "\xe2\x9a\xa1","Loops","55 MB"},
        {"Gospel Choir",        "SATB choir – runs, chords, backgrounds",  "\xf0\x9f\x8e\xa4","Instruments","200 MB"},
        {"World Percussion",    "Djembe, congas, talking drum & more",     "\xf0\x9f\xaa\x98","Drums","80 MB"},
        {"Future Bass Chords",  "Supersaw stabs and arpeggios",            "\xf0\x9f\x8c\x8a","Instruments","20 MB"},
        {"Vinyl Crackle",       "Authentic vinyl noise textures",          "\xf0\x9f\x93\x80","Effects","5 MB"},
    };

    searchBox.setTextToShowWhenEmpty ("Search plugins...", TDIM);
    searchBox.onTextChange = [this] {
        filterPlugins (activeCat, searchBox.getText());
        rebuildList();
    };
    addAndMakeVisible (searchBox);

    for (int i = 0; i < 5; ++i)
    {
        catBtns[i].setButtonText (CATS[i]);
        catBtns[i].setColour (juce::TextButton::buttonColourId, col (TurboLookAndFeel::bgCard));
        catBtns[i].setColour (juce::TextButton::textColourOffId, TDIM);
        catBtns[i].setColour (juce::TextButton::buttonOnColourId, GOLD.withAlpha (0.8f));
        int ci = i;
        catBtns[i].onClick = [this, ci] {
            activeCat = ci;
            for (int j = 0; j < 5; ++j)
                catBtns[j].setColour (juce::TextButton::buttonColourId,
                    j == ci ? GOLD.withAlpha (0.8f) : col (TurboLookAndFeel::bgCard));
            filterPlugins (ci, searchBox.getText());
            rebuildList();
        };
        addAndMakeVisible (catBtns[i]);
    }
    catBtns[0].setColour (juce::TextButton::buttonColourId, GOLD.withAlpha (0.8f));

    viewport.setScrollBarsShown (true, false);
    viewport.setViewedComponent (&listContainer, false);
    addAndMakeVisible (viewport);

    filterPlugins (0, "");
    rebuildList();
    startTimerHz (10);
}

void PluginStorePanel::filterPlugins (int cat, const juce::String& search)
{
    filteredIdx.clear();
    juce::String catName = cat == 0 ? "" : CATS[cat];
    for (int i = 0; i < (int)plugins.size(); ++i)
    {
        bool matchCat  = cat == 0 || plugins[i].category.equalsIgnoreCase (catName);
        bool matchText = search.isEmpty()
            || plugins[i].name.containsIgnoreCase (search)
            || plugins[i].desc.containsIgnoreCase (search);
        if (matchCat && matchText) filteredIdx.push_back (i);
    }
}

void PluginStorePanel::rebuildList()
{
    listContainer.removeAllChildren();
    dlBtns.clear();

    int rowH = 72;
    listContainer.setSize (viewport.getWidth() > 0 ? viewport.getWidth() : 400,
                           (int)filteredIdx.size() * rowH);

    for (int fi = 0; fi < (int)filteredIdx.size(); ++fi)
    {
        int pi = filteredIdx[fi];
        auto* btn = new juce::TextButton (plugins[pi].installed ? "Installed" : "Get FREE");
        btn->setColour (juce::TextButton::buttonColourId,
                        plugins[pi].installed ? GREEN.withAlpha (0.5f) : GOLD.withAlpha (0.8f));
        btn->setColour (juce::TextButton::textColourOffId,
                        plugins[pi].installed ? TEXT : juce::Colours::black);
        if (!plugins[pi].installed)
        {
            int fii = fi;
            btn->onClick = [this, fii] {
                if (fii < (int)filteredIdx.size())
                    startDownload (filteredIdx[fii]);
            };
        }
        btn->setBounds (listContainer.getWidth() - 110, fi * rowH + 20, 100, 30);
        listContainer.addAndMakeVisible (btn);
        dlBtns.add (btn);
    }
    listContainer.repaint();
}

void PluginStorePanel::startDownload (int pi)
{
    if (pi >= (int)plugins.size() || plugins[pi].installed || plugins[pi].downloading) return;
    plugins[pi].downloading = true;
    plugins[pi].dlProgress  = 0.f;
    if (onStatus) onStatus ("Downloading " + plugins[pi].name + "...");
}

void PluginStorePanel::timerCallback()
{
    bool any = false;
    for (auto& p : plugins)
    {
        if (p.downloading)
        {
            p.dlProgress += 0.04f;
            if (p.dlProgress >= 1.f)
            {
                p.downloading = false;
                p.installed   = true;
                if (onStatus) onStatus (p.name + " installed!");
                rebuildList();
            }
            any = true;
        }
    }
    if (any) listContainer.repaint();
}

void PluginStorePanel::paint (juce::Graphics& g)
{
    int rowH = 72;
    for (int fi = 0; fi < (int)filteredIdx.size(); ++fi)
    {
        int pi = filteredIdx[fi];
        auto& plug = plugins[pi];
        auto row = juce::Rectangle<int> (0, fi * rowH, listContainer.getWidth(), rowH - 4);

        // Card
        g.setColour (col (TurboLookAndFeel::bgPanel));
        g.fillRoundedRectangle (row.toFloat(), 8.f);
        g.setColour (BORDER);
        g.drawRoundedRectangle (row.toFloat().reduced (0.5f), 8.f, 1.f);

        // Emoji
        g.setFont (28.f);
        g.setColour (TEXT);
        g.drawText (plug.emoji, row.withWidth (52), juce::Justification::centred);

        // Name & desc
        auto textArea = row.withLeft (58).withRight (row.getRight() - 120);
        g.setFont (juce::Font (14.f, juce::Font::bold));
        g.setColour (TEXT);
        g.drawText (plug.name, textArea.removeFromTop (26), juce::Justification::centredLeft);
        g.setFont (11.f);
        g.setColour (TDIM);
        g.drawText (plug.desc + "  •  " + plug.category + "  •  " + plug.size,
                    textArea, juce::Justification::centredLeft);

        // Progress bar
        if (plug.downloading)
        {
            auto barArea = row.withLeft (58).withRight (row.getRight() - 120)
                              .withTop (row.getBottom() - 10).withHeight (6);
            g.setColour (col(TurboLookAndFeel::bgInput));
            g.fillRoundedRectangle (barArea.toFloat(), 3.f);
            g.setColour (GOLD);
            g.fillRoundedRectangle (barArea.withWidth ((int)(barArea.getWidth() * plug.dlProgress)).toFloat(), 3.f);
        }
    }
}

void PluginStorePanel::resized()
{
    auto area = getLocalBounds();
    searchBox.setBounds (area.removeFromTop (36).reduced (2, 4));
    auto catRow = area.removeFromTop (36);
    int cw = catRow.getWidth() / 5;
    for (int i = 0; i < 5; ++i)
        catBtns[i].setBounds (catRow.removeFromLeft (cw).reduced (2, 4));

    viewport.setBounds (area);
    int rowH = 72;
    listContainer.setSize (area.getWidth(), (int)filteredIdx.size() * rowH + 4);
    rebuildList();
}

} // namespace TurboTuning
