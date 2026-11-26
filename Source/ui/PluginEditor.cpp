#include "PluginEditor.h"
#include "engine/DSPCore.h"

using namespace ui;

//============================================================================== widgets
Knob::Knob (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& l, ShowFn fn)
    : label (l), onShow (std::move (fn))
{
    param = s.getParameter (id);
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setMouseDragSensitivity (180);
    slider.setVelocityBasedMode (false);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.22f, juce::MathConstants<float>::pi * 2.78f, true);
    addAndMakeVisible (slider);
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (s, id, slider);
    if (param != nullptr)
    {
        slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
        auto* p = param;
        slider.textFromValueFunction = [p] (double v) { return p->getText (p->convertTo0to1 ((float) v), 12); };
    }
    slider.onValueChange = [this]
    {
        repaint();
        if (onShow && slider.isMouseButtonDown()) onShow (label, slider.getTextFromValue (slider.getValue()));
    };
    slider.onDragStart = [this] { if (onShow) onShow (label, slider.getTextFromValue (slider.getValue())); };
    slider.addMouseListener (this, false);
}

void Knob::resized()
{
    auto r = getLocalBounds();
    slider.setBounds (r.removeFromTop (r.getHeight() - 16));
}

void Knob::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().removeFromBottom (16);
    const bool show = slider.isMouseOverOrDragging();
    g.setFont (fonts::mono (show ? 10.5f : 10.0f, true));
    g.setColour (! isEnabled() ? pal::dim.withAlpha (0.5f) : (show ? pal::text : pal::dim.brighter (0.35f)));
    g.drawText (show ? slider.getTextFromValue (slider.getValue()) : label, r, juce::Justification::centred, false);
}

Choice::Choice (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& l) : label (l)
{
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (s.getParameter (id)))
        box.addItemList (p->choices, 1);
    addAndMakeVisible (box);
    att = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, id, box);
}

void Choice::resized()
{
    auto r = getLocalBounds();
    if (label.isNotEmpty()) r.removeFromTop (13);
    box.setBounds (r.withHeight (juce::jmin (r.getHeight(), 20)));
}

void Choice::paint (juce::Graphics& g)
{
    if (label.isEmpty()) return;
    g.setFont (fonts::mono (9.5f, true));
    g.setColour (pal::dim.brighter (0.35f));
    g.drawText (label, getLocalBounds().removeFromTop (13), juce::Justification::centredLeft, false);
}

Led::Led (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& text)
{
    setButtonText (text);
    att = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, id, *this);
}

int Section::headerTextWidth() const
{
    return juce::GlyphArrangement::getStringWidthInt (fonts::mono (11.0f, true), index + " " + title) + 22;
}

void Section::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (pal::panel);
    g.fillRect (r);
    g.setColour (pal::line);
    g.drawRect (r, 1.0f);
    auto h = r.removeFromTop (20.0f);
    g.setColour (pal::panel2);
    g.fillRect (h.reduced (1.0f, 1.0f));
    g.setColour (pal::line);
    g.drawHorizontalLine ((int) h.getBottom(), h.getX(), h.getRight());
    auto* l = dynamic_cast<DesgraciaLookAndFeel*> (&getLookAndFeel());
    g.setColour (l != nullptr ? l->getAccent() : pal::acid);
    g.fillRect (h.getX() + 1.0f, h.getY() + 1.0f, 3.0f, h.getHeight() - 2.0f);
    g.setFont (fonts::mono (11.0f, true));
    g.setColour (pal::dim);
    g.drawText (index, h.withTrimmedLeft (10.0f), juce::Justification::centredLeft, false);
    if (hideTitle) return;
    g.setColour (pal::text);
    g.drawText (title, h.withTrimmedLeft (12.0f + (float) juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), index + " ")),
                juce::Justification::centredLeft, false);
}

void WaveView::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (pal::bg);
    g.fillRect (r);
    g.setColour (pal::line);
    g.drawRect (r, 1.0f);
    g.drawHorizontalLine ((int) r.getCentreY(), r.getX() + 2, r.getRight() - 2);
    dsp::Osc o;
    const int N = 240;
    juce::Path p;
    for (int i = 0; i < N; ++i)
    {
        const float ph = std::fmod (2.0f * (float) i / (float) N, 1.0f);   // two cycles
        o.p = ph; o.q = std::fmod (2.0f * (float) i / (float) N * 2.76f, 1.0f);
        const float y = juce::jlimit (-1.2f, 1.2f, o.tick (wave, 0.004f, shape, 0.0f));
        const float x = r.getX() + 3 + (r.getWidth() - 6) * (float) i / (float) (N - 1);
        const float yy = r.getCentreY() - y * (r.getHeight() * 0.38f);
        if (i == 0) p.startNewSubPath (x, yy); else p.lineTo (x, yy);
    }
    auto* l = dynamic_cast<DesgraciaLookAndFeel*> (&getLookAndFeel());
    g.setColour (l != nullptr ? l->getAccent() : pal::acid);
    g.strokePath (p, juce::PathStrokeType (1.6f));
    if (fmAmt > 0.001f)
    {
        g.setColour (pal::hot);
        g.setFont (fonts::mono (9.0f, true));
        g.drawText ("FM", r.reduced (4, 2), juce::Justification::topRight);
    }
}

void EnvView::set (float a, float d, float s, float r)
{
    if (a == A && d == D && s == S && r == R) return;
    A = a; D = d; S = s; R = r; repaint();
}

void EnvView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (pal::bg); g.fillRect (b);
    g.setColour (pal::line); g.drawRect (b, 1.0f);
    b = b.reduced (6.0f, 8.0f);
    auto tw = [] (float t) { return std::sqrt (t / 8.0f); };   // perceptual time axis
    const float total = tw (A) + tw (D) + 0.25f + tw (R);
    const float k = b.getWidth() / total;
    float x = b.getX();
    juce::Path p;
    p.startNewSubPath (x, b.getBottom());
    x += tw (A) * k; p.lineTo (x, b.getY());
    const float sy = b.getBottom() - S * b.getHeight();
    const float x2 = x + tw (D) * k;
    p.quadraticTo (x + (x2 - x) * 0.25f, sy, x2, sy);
    x = x2 + 0.25f * k; p.lineTo (x, sy);
    const float x3 = x + tw (R) * k;
    p.quadraticTo (x + (x3 - x) * 0.25f, b.getBottom(), x3, b.getBottom());
    auto* l = dynamic_cast<DesgraciaLookAndFeel*> (&getLookAndFeel());
    const auto c = l != nullptr ? l->getAccent() : pal::acid;
    juce::Path fill (p); fill.lineTo (b.getX(), b.getBottom()); fill.closeSubPath();
    g.setColour (c.withAlpha (0.08f)); g.fillPath (fill);
    g.setColour (c); g.strokePath (p, juce::PathStrokeType (1.6f));
}

void FilterView::set (int t, float c, float r)
{
    if (t == type && std::abs (c - cut) < 0.5f && std::abs (r - res) < 0.001f) return;
    type = t; cut = c; res = r; repaint();
}

void FilterView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (pal::bg); g.fillRect (b);
    g.setColour (pal::line); g.drawRect (b, 1.0f);
    for (float f : { 100.0f, 1000.0f, 10000.0f })
    {
        const float x = b.getX() + b.getWidth() * std::log (f / 20.0f) / std::log (1000.0f);
        g.drawVerticalLine ((int) x, b.getY() + 1, b.getBottom() - 1);
    }
    b = b.reduced (2.0f, 4.0f);
    const float k = type == 4 ? 2.0f - 1.6f * res : 2.0f - 1.96f * res;
    juce::Path p;
    const int N = 90;
    for (int i = 0; i < N; ++i)
    {
        const float f = 20.0f * std::pow (1000.0f, (float) i / (float) (N - 1));
        float mag = 1.0f;
        auto svf = [&] (float fc, float kk, int mode)
        {
            const float w = f / fc, den = std::sqrt ((1 - w * w) * (1 - w * w) + kk * kk * w * w);
            switch (mode)
            {
                case 0: return 1.0f / den;
                case 1: return kk * w / den;
                case 2: return w * w / den;
                default: return std::abs (1 - w * w) / den;
            }
        };
        switch (type)
        {
            case 0: mag = svf (cut, k, 0); break;
            case 1: mag = svf (cut, 2.0f - 1.1f * res, 0) * svf (cut, 2.0f - 1.93f * res, 0); break;
            case 2: mag = svf (cut, k, 1) * (1.0f + 1.5f * res); break;
            case 3: mag = svf (cut, k, 2); break;
            case 4: mag = svf (cut, k, 3); break;
            case 5:
            {
                const float pos = juce::jlimit (0.0f, 1.0f, (std::log2 (cut) - 7.64f) / (12.29f - 7.64f));
                const float q = 1.0f / (3.0f + 17.0f * res);
                mag = 0.0f;
                const float f1 = 300.0f + 500.0f * (1.0f - std::abs (pos - 0.2f) * 1.2f);
                const float f2 = 700.0f + 1600.0f * std::sin (pos * 3.1f);
                mag = 2.2f * (svf (juce::jmax (150.0f, f1), q, 1) + 0.5f * svf (juce::jmax (300.0f, f2), q, 1) + 0.25f * svf (2800.0f, q, 1));
                break;
            }
            default: break;
        }
        const float db = juce::jlimit (-36.0f, 18.0f, 20.0f * std::log10 (juce::jmax (1.0e-4f, mag)));
        const float x = b.getX() + b.getWidth() * (float) i / (float) (N - 1);
        const float y = b.getY() + b.getHeight() * (1.0f - (db + 36.0f) / 54.0f);
        if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
    }
    auto* l = dynamic_cast<DesgraciaLookAndFeel*> (&getLookAndFeel());
    g.setColour (l != nullptr ? l->getAccent() : pal::acid);
    g.strokePath (p, juce::PathStrokeType (1.6f));
}

void LevelMeter::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (pal::bg); g.fillRect (b);
    const int segs = 16;
    const float db = 20.0f * std::log10 (juce::jmax (1.0e-5f, level * 1.414f));
    const float norm = juce::jlimit (0.0f, 1.0f, (db + 48.0f) / 48.0f);
    const float sh = (b.getHeight() - 4.0f) / (float) segs;
    for (int i = 0; i < segs; ++i)
    {
        const bool on = (float) i / (float) segs < norm;
        const auto c = i > 13 ? pal::hot : (i > 10 ? juce::Colour (0xffffd23a) : pal::acid);
        g.setColour (on ? c : pal::line);
        g.fillRect (b.getX() + 2.0f, b.getBottom() - 2.0f - (float) (i + 1) * sh + 1.0f, b.getWidth() - 4.0f, sh - 1.5f);
    }
}

void DesgraciaEditor::Content::paint (juce::Graphics& g)
{
    g.fillAll (pal::bg);
    // logo
    auto* l = dynamic_cast<DesgraciaLookAndFeel*> (&getLookAndFeel());
    const auto acc = l != nullptr ? l->getAccent() : pal::acid;
    g.setColour (acc);
    g.fillRect (12, 12, 8, 28);
    g.setFont (fonts::mono (27.0f, true));
    g.setColour (pal::text);
    g.drawText ("DESGRACIA", 26, 6, 220, 30, juce::Justification::centredLeft, false);
    g.setFont (fonts::mono (9.5f, true));
    g.setColour (pal::dim);
    g.drawText ("SINTE DIGITAL SUCIO // v1.0", 28, 33, 220, 12, juce::Justification::centredLeft, false);
    g.setColour (pal::line);
    g.drawHorizontalLine (50, 12.0f, (float) getWidth() - 12.0f);
    // faint dot matrix
    g.setColour (pal::line.withAlpha (0.25f));
    for (int y = 56; y < getHeight(); y += 12)
        for (int x = 6; x < getWidth(); x += 12)
            g.fillRect (x, y, 1, 1);
}

//============================================================================== editor
ui::Knob& DesgraciaEditor::knob (const juce::String& id, const juce::String& label)
{
    auto* k = knobs.add (new ui::Knob (proc.apvts, id, label, [this] (const juce::String& n, const juce::String& v) { showValue (n, v); }));
    content.addAndMakeVisible (k);
    knobMap[id] = k;
    return *k;
}

ui::Choice& DesgraciaEditor::choice (const juce::String& id, const juce::String& label)
{
    auto* c = choicesList.add (new ui::Choice (proc.apvts, id, label));
    content.addAndMakeVisible (c);
    choiceMap[id] = c;
    return *c;
}

ui::Led& DesgraciaEditor::led (const juce::String& id, const juce::String& text)
{
    auto* l = leds.add (new ui::Led (proc.apvts, id, text));
    content.addAndMakeVisible (l);
    l->onClick = [this, l] { if (l->isMouseOver (true)) showValue (l->getButtonText(), l->getToggleState() ? "ON" : "OFF"); };
    return *l;
}

ui::Section& DesgraciaEditor::section (const juce::String& idx, const juce::String& title)
{
    auto* s = sections.add (new ui::Section (idx, title));
    content.addAndMakeVisible (s);
    s->toBack();
    sectionMap[title] = s;
    return *s;
}

float DesgraciaEditor::param (const juce::String& id) const
{
    auto* r = proc.apvts.getRawParameterValue (id);
    return r != nullptr ? r->load() : 0.0f;
}

DesgraciaEditor::DesgraciaEditor (DesgraciaProcessor& p)
    : AudioProcessorEditor (p), proc (p), character (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (content);
    content.setSize (baseW, baseH);

    const char* secs[][2] = { { "01", "OSC A" }, { "02", "OSC B" }, { "03", "B>A MOD / PITCH ENV" },
                              { "04", "SOURCE" }, { "05", "FILTER" }, { "06", "VOICE / OUT" },
                              { "07", "AMP ENV" }, { "08", "FILTER ENV  (also FM ENV)" },
                              { "09", "LFO 1  (+MOD WHEEL)" }, { "10", "LFO 2" },
                              { "11", "DRIVE" }, { "12", "CRUSH" }, { "13", "EQ" }, { "14", "CHORUS" }, { "15", "DELAY" }, { "16", "REVERB" } };
    for (auto& s : secs) section (s[0], s[1]);
    for (auto* t : { "DRIVE", "CRUSH", "EQ", "CHORUS", "DELAY", "REVERB" }) sectionMap[t]->hideTitle = true;

    // OSC A / B
    for (auto t : { "A", "B" })
    {
        juce::String o = juce::String ("osc") + t + "_";
        choice (o + "wave");
        knob (o + "oct", "OCT"); knob (o + "semi", "SEMI"); knob (o + "fine", "FINE");
        knob (o + "shape", "SHAPE"); knob (o + "level", "LEVEL");
    }
    content.addAndMakeVisible (waveA);
    content.addAndMakeVisible (waveB);

    knob (pid::fmAmt, "FM B>A"); knob (pid::fmEnv, "FM ENV"); knob (pid::ringAmt, "RING");
    knob (pid::penvAmt, "P.ENV"); knob (pid::penvDecay, "P.DECAY"); choice (pid::penvDest, "P.ENV TO");

    knob (pid::subLevel, "SUB"); choice (pid::subWave, "SUB WAVE"); choice (pid::subOct, "SUB OCT");
    knob (pid::noiseLevel, "NOISE"); knob (pid::noiseColor, "COLOR");
    knob (pid::uniVoices, "UNISON"); knob (pid::uniDetune, "DETUNE"); knob (pid::uniSpread, "SPREAD"); knob (pid::drift, "DRIFT");

    choice (pid::fltType);
    knob (pid::fltCutoff, "CUTOFF"); knob (pid::fltReso, "RESO"); knob (pid::fltDrive, "DRIVE");
    knob (pid::fltEnv, "ENV"); knob (pid::fltKey, "KEY"); knob (pid::fltVel, "VEL");
    content.addAndMakeVisible (filterView);

    choice (pid::voiceMode, "MODE"); knob (pid::glide, "GLIDE"); knob (pid::bendRange, "BEND");
    knob (pid::width, "WIDTH"); knob (pid::master, "MASTER");
    content.addAndMakeVisible (meter);

    knob (pid::ampA, "A"); knob (pid::ampD, "D"); knob (pid::ampS, "S"); knob (pid::ampR, "R"); knob (pid::ampVel, "VEL");
    knob (pid::fenvA, "A"); knob (pid::fenvD, "D"); knob (pid::fenvS, "S"); knob (pid::fenvR, "R");
    content.addAndMakeVisible (ampView);
    content.addAndMakeVisible (fenvView);

    for (int i = 0; i < 2; ++i)
    {
        choice (pid::lfo (i, "shape"));
        knob (pid::lfo (i, "rate"), "RATE"); knob (pid::lfo (i, "amt"), "AMOUNT"); knob (pid::lfo (i, "delay"), "DELAY");
        choice (pid::lfo (i, "dest"), "DEST"); choice (pid::lfo (i, "div"), "SYNC DIV");
        led (pid::lfo (i, "sync"), "SYNC");
    }

    led (pid::drvOn, "DRIVE"); choice (pid::drvType, "TYPE"); knob (pid::drvAmt, "AMOUNT"); knob (pid::drvMix, "MIX");
    led (pid::crsOn, "CRUSH"); knob (pid::crsBits, "BITS"); knob (pid::crsRate, "RATE"); knob (pid::crsMix, "MIX");
    led (pid::eqOn, "EQ"); knob (pid::eqLow, "LOW"); knob (pid::eqMid, "MID"); knob (pid::eqMidFreq, "FREQ"); knob (pid::eqHigh, "HIGH");
    led (pid::choOn, "CHORUS"); knob (pid::choRate, "RATE"); knob (pid::choDepth, "DEPTH"); knob (pid::choMix, "MIX");
    led (pid::dlyOn, "DELAY"); led (pid::dlySync, "SYNC"); led (pid::dlyPing, "PING"); choice (pid::dlyDiv);
    knob (pid::dlyTime, "TIME"); knob (pid::dlyFb, "FDBK"); knob (pid::dlyTone, "TONE"); knob (pid::dlyMix, "MIX");
    led (pid::revOn, "REVERB"); knob (pid::revSize, "SIZE"); knob (pid::revDamp, "DAMP"); knob (pid::revMix, "MIX");

    content.addAndMakeVisible (character);

    keyboard.setAvailableRange (12, 108);
    keyboard.setLowestVisibleKey (36);
    keyboard.setKeyWidth (24.0f);
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xffcfcfc4));
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, pal::panel2);
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, pal::bg);
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, pal::acid.withAlpha (0.35f));
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, pal::acid);
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::transparentBlack);
    keyboard.setColour (juce::MidiKeyboardComponent::upDownButtonBackgroundColourId, pal::panel2);
    keyboard.setColour (juce::MidiKeyboardComponent::upDownButtonArrowColourId, pal::text);
    keyboard.setWantsKeyboardFocus (false);
    content.addAndMakeVisible (keyboard);

    // preset bar
    categoryBox.addItem ("ALL", 1);
    categoryBox.addItemList (PresetManager::getCategories(), 2);
    categoryBox.setSelectedId (1, juce::dontSendNotification);
    categoryBox.onChange = [this]
    {
        const auto cat = categoryBox.getText();
        if (cat != "ALL" && proc.presets.getCurrentCategory() != cat) proc.presets.step (1, cat);
    };
    prevBtn.onClick = [this] { proc.presets.step (-1, categoryBox.getText()); };
    nextBtn.onClick = [this] { proc.presets.step (1, categoryBox.getText()); };
    nameBtn.onClick = [this] { showPresetMenu(); };
    randBtn.onClick = [this] { proc.presets.randomize(); };
    saveBtn.onClick = [this] { savePresetDialog(); };
    initBtn.onClick = [this] { proc.presets.initPatch(); };
    randBtn.setTooltip ("random patch (inside musical ranges)");
    for (juce::Component* c : { (juce::Component*) &categoryBox, (juce::Component*) &prevBtn, (juce::Component*) &nextBtn,
                                (juce::Component*) &nameBtn, (juce::Component*) &randBtn, (juce::Component*) &saveBtn,
                                (juce::Component*) &initBtn, (juce::Component*) &status })
        content.addAndMakeVisible (c);
    status.setFont (fonts::mono (12.0f, true));
    status.setColour (juce::Label::textColourId, pal::dim);
    status.setJustificationType (juce::Justification::centredRight);

    proc.presets.onChanged = [this] { presetDirty = true; };

    layoutContent();

    setResizable (true, true);
    setResizeLimits (800, 500, 1920, 1200);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) baseW / (double) baseH);
    setSize (1120, 700);
    startTimerHz (20);
}

DesgraciaEditor::~DesgraciaEditor()
{
    proc.presets.onChanged = nullptr;
    stopTimer();
    setLookAndFeel (nullptr);
}

void DesgraciaEditor::paint (juce::Graphics& g) { g.fillAll (pal::bg); }

void DesgraciaEditor::resized()
{
    const float s = (float) getWidth() / (float) baseW;
    content.setTransform (juce::AffineTransform::scale (s));
}

void DesgraciaEditor::layoutContent()
{
    constexpr int kw = 54, kh = 62;
    auto place = [&] (const juce::String& id, int x, int y) { if (auto* k = knobMap[id]) k->setBounds (x, y, kw, kh); };
    auto row = [&] (std::initializer_list<juce::String> ids, int x, int y, int gap = 54)
    { for (auto& id : ids) { place (id, x, y); x += gap; } };
    auto sec = [&] (const juce::String& title, int x, int y, int w, int h) -> juce::Rectangle<int>
    { auto* s = sectionMap[title]; s->setBounds (x, y, w, h); return s->getBounds(); };

    // header
    categoryBox.setBounds (300, 14, 150, 24);
    prevBtn.setBounds (458, 14, 28, 24);
    nameBtn.setBounds (490, 14, 300, 24);
    nextBtn.setBounds (794, 14, 28, 24);
    randBtn.setBounds (832, 14, 56, 24);
    saveBtn.setBounds (892, 14, 56, 24);
    initBtn.setBounds (952, 14, 56, 24);
    status.setBounds (1016, 14, 252, 24);

    const int c1 = 12, c2 = 320, c3 = 628, r1 = 58, r2 = 244, r3 = 430;

    // OSC A / B
    for (int i = 0; i < 2; ++i)
    {
        auto b = sec (i == 0 ? "OSC A" : "OSC B", i == 0 ? c1 : c2, r1, 300, 178);
        juce::String o = i == 0 ? "oscA_" : "oscB_";
        choiceMap[o + "wave"]->setBounds (b.getRight() - 106, b.getY() + 2, 102, 16);
        (i == 0 ? waveA : waveB).setBounds (b.getX() + 8, b.getY() + 28, b.getWidth() - 16, 62);
        row ({ o + "oct", o + "semi", o + "fine", o + "shape", o + "level" }, b.getX() + 15, b.getY() + 104);
    }

    {
        auto b = sec ("B>A MOD / PITCH ENV", c3, r1, 292, 178);
        row ({ pid::fmAmt, pid::fmEnv, pid::ringAmt }, b.getX() + 20, b.getY() + 30, 84);
        row ({ pid::penvAmt, pid::penvDecay }, b.getX() + 20, b.getY() + 106, 84);
        choiceMap[pid::penvDest]->setBounds (b.getX() + 190, b.getY() + 116, 84, 34);
    }
    {
        auto b = sec ("SOURCE", c1, r2, 300, 178);
        place (pid::subLevel, b.getX() + 12, b.getY() + 28);
        choiceMap[pid::subWave]->setBounds (b.getX() + 72, b.getY() + 26, 76, 33);
        choiceMap[pid::subOct]->setBounds (b.getX() + 72, b.getY() + 60, 76, 33);
        row ({ pid::noiseLevel, pid::noiseColor }, b.getX() + 168, b.getY() + 28, 62);
        row ({ pid::uniVoices, pid::uniDetune, pid::uniSpread, pid::drift }, b.getX() + 12, b.getY() + 106, 70);
    }
    {
        auto b = sec ("FILTER", c2, r2, 300, 178);
        choiceMap[pid::fltType]->setBounds (b.getRight() - 86, b.getY() + 2, 82, 16);
        row ({ pid::fltCutoff, pid::fltReso, pid::fltDrive }, b.getX() + 8, b.getY() + 28);
        row ({ pid::fltEnv, pid::fltKey, pid::fltVel }, b.getX() + 8, b.getY() + 106);
        filterView.setBounds (b.getX() + 176, b.getY() + 30, 116, 138);
    }
    {
        auto b = sec ("VOICE / OUT", c3, r2, 292, 178);
        choiceMap[pid::voiceMode]->setBounds (b.getX() + 14, b.getY() + 34, 90, 34);
        row ({ pid::glide, pid::bendRange }, b.getX() + 116, b.getY() + 28, 62);
        row ({ pid::width, pid::master }, b.getX() + 116, b.getY() + 106, 62);
        meter.setBounds (b.getRight() - 36, b.getY() + 30, 22, 138);
    }
    {
        auto b = sec ("AMP ENV", c1, r3, 446, 150);
        ampView.setBounds (b.getX() + 8, b.getY() + 28, 152, 112);
        row ({ pid::ampA, pid::ampD, pid::ampS, pid::ampR, pid::ampVel }, b.getX() + 170, b.getY() + 50);
    }
    {
        auto b = sec ("FILTER ENV  (also FM ENV)", 466, r3, 454, 150);
        fenvView.setBounds (b.getX() + 8, b.getY() + 28, 200, 112);
        row ({ pid::fenvA, pid::fenvD, pid::fenvS, pid::fenvR }, b.getX() + 222, b.getY() + 50);
    }

    character.setBounds (928, 58, 340, 290);

    for (int i = 0; i < 2; ++i)
    {
        auto b = sec (i == 0 ? "LFO 1  (+MOD WHEEL)" : "LFO 2", 928, i == 0 ? 356 : 472, 340, i == 0 ? 110 : 108);
        choiceMap[pid::lfo (i, "shape")]->setBounds (b.getRight() - 86, b.getY() + 2, 82, 16);
        row ({ pid::lfo (i, "rate"), pid::lfo (i, "amt"), pid::lfo (i, "delay") }, b.getX() + 8, b.getY() + 30);
        choiceMap[pid::lfo (i, "dest")]->setBounds (b.getX() + 178, b.getY() + 26, 150, 34);
        choiceMap[pid::lfo (i, "div")]->setBounds (b.getX() + 252, b.getY() + 62, 76, 34);
    }
    // LFO sync leds are the first two leds created
    leds[0]->setBounds (928 + 178, 356 + 76, 64, 18);
    leds[1]->setBounds (928 + 178, 472 + 76, 64, 18);

    // FX row
    {
        const int y = 588, h = 120, ky = y + 34;
        int x = 12;
        auto fxsec = [&] (const juce::String& title, int w) { auto b = sec (title, x, y, w, h); x += w + 8; return b; };
        auto ledHead = [&] (int index, juce::Rectangle<int> b) { leds[index]->setBounds (b.getX() + 30, b.getY() + 1, 100, 18); };

        auto b = fxsec ("DRIVE", 190); ledHead (2, b);
        choiceMap[pid::drvType]->setBounds (b.getX() + 8, ky + 8, 68, 34);
        row ({ pid::drvAmt, pid::drvMix }, b.getX() + 80, ky);
        b = fxsec ("CRUSH", 180); ledHead (3, b);
        row ({ pid::crsBits, pid::crsRate, pid::crsMix }, b.getX() + 9, ky);
        b = fxsec ("EQ", 232); ledHead (4, b);
        row ({ pid::eqLow, pid::eqMid, pid::eqMidFreq, pid::eqHigh }, b.getX() + 8, ky);
        b = fxsec ("CHORUS", 180); ledHead (5, b);
        row ({ pid::choRate, pid::choDepth, pid::choMix }, b.getX() + 9, ky);
        b = fxsec ("DELAY", 250); ledHead (6, b);
        leds[7]->setBounds (b.getX() + 8, b.getBottom() - 22, 56, 18);
        leds[8]->setBounds (b.getX() + 66, b.getBottom() - 22, 56, 18);
        choiceMap[pid::dlyDiv]->setBounds (b.getRight() - 82, b.getY() + 2, 78, 16);
        row ({ pid::dlyTime, pid::dlyFb, pid::dlyTone, pid::dlyMix }, b.getX() + 17, ky - 8);
        b = fxsec ("REVERB", 180); ledHead (9, b);
        row ({ pid::revSize, pid::revDamp, pid::revMix }, b.getX() + 9, ky);
    }

    keyboard.setBounds (12, 716, 1256, 76);
}

void DesgraciaEditor::showValue (const juce::String& name, const juce::String& value)
{
    status.setText (name + " = " + value, juce::dontSendNotification);
    status.setColour (juce::Label::textColourId, lnf.getAccent());
    statusHold = 40;
    character.say (name + " " + value, 1.0f);
}

void DesgraciaEditor::refreshPresetDisplay()
{
    const auto name = proc.presets.getCurrentName();
    const auto cat = proc.presets.getCurrentCategory();
    nameBtn.setButtonText (name + (cat.isNotEmpty() && cat != "-" ? "   [" + cat + "]" : juce::String()));
    const auto c = pal::forCategory (cat);
    lnf.setAccent (c);
    character.setTint (c);
    if (name != lastShownPreset)
    {
        character.say (proc.presets.getCurrentBlurb(), 2.2f);
        character.blink();
        lastShownPreset = name;
    }
    content.repaint();
    repaint();
}

void DesgraciaEditor::timerCallback()
{
    if (presetDirty.exchange (false)) refreshPresetDisplay();

    const int wA = (int) param (pid::oscAWave), wB = (int) param (pid::oscBWave);
    waveA.set (wA, param (pid::oscAShape), param (pid::fmAmt) + param (pid::fmEnv), true);
    waveB.set (wB, param (pid::oscBShape), 0.0f, false);
    knobMap[pid::oscAShape]->setLabel (choices::shapeName (wA));
    knobMap[pid::oscBShape]->setLabel (choices::shapeName (wB));
    ampView.set (param (pid::ampA), param (pid::ampD), param (pid::ampS), param (pid::ampR));
    fenvView.set (param (pid::fenvA), param (pid::fenvD), param (pid::fenvS), param (pid::fenvR));
    filterView.set ((int) param (pid::fltType), param (pid::fltCutoff), param (pid::fltReso));
    meter.set (proc.outputLevel.load());

    for (int i = 0; i < 2; ++i)
    {
        const bool sync = param (pid::lfo (i, "sync")) > 0.5f;
        knobMap[pid::lfo (i, "rate")]->setEnabled (! sync);
        knobMap[pid::lfo (i, "rate")]->setAlpha (sync ? 0.35f : 1.0f);
        choiceMap[pid::lfo (i, "div")]->setEnabled (sync);
        choiceMap[pid::lfo (i, "div")]->setAlpha (sync ? 1.0f : 0.35f);
    }
    const bool dsync = param (pid::dlySync) > 0.5f;
    knobMap[pid::dlyTime]->setEnabled (! dsync);
    knobMap[pid::dlyTime]->setAlpha (dsync ? 0.35f : 1.0f);
    choiceMap[pid::dlyDiv]->setEnabled (dsync);
    choiceMap[pid::dlyDiv]->setAlpha (dsync ? 1.0f : 0.35f);

    if (statusHold > 0 && --statusHold == 0)
    {
        status.setText (juce::String (proc.engine.activeVoices.load()) + " VOICES", juce::dontSendNotification);
        status.setColour (juce::Label::textColourId, pal::dim);
    }
    else if (statusHold == 0)
        status.setText (juce::String (proc.engine.activeVoices.load()) + " VOICES", juce::dontSendNotification);
}

void DesgraciaEditor::showPresetMenu()
{
    juce::PopupMenu m;
    const auto& entries = proc.presets.getEntries();
    const auto filter = categoryBox.getText();
    for (auto& cat : PresetManager::getCategories())
    {
        if (filter != "ALL" && filter != cat) continue;
        bool any = false;
        for (int i = 0; i < entries.size(); ++i)
            if (entries[i].category == cat)
            {
                if (! any) { m.addSectionHeader (cat); any = true; }
                m.addItem (i + 1, entries[i].name, true, i == proc.presets.getCurrentIndex());
            }
    }
    m.addSeparator();
    m.addItem (10000, "rescan user presets");
    m.addItem (10001, "open user preset folder");
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&nameBtn).withMinimumWidth (nameBtn.getWidth()),
                     [this] (int r)
                     {
                         if (r == 10000) { proc.presets.rescanUser(); return; }
                         if (r == 10001)
                         {
                             auto d = PresetManager::getUserDirectory();
                             d.createDirectory();
                             d.startAsProcess();
                             return;
                         }
                         if (r > 0) proc.presets.load (r - 1);
                     });
}

void DesgraciaEditor::savePresetDialog()
{
    auto* w = new juce::AlertWindow ("GUARDAR PRESET", "Nombre del preset (se guarda en Documentos/Desgracia/Presets):",
                                     juce::MessageBoxIconType::NoIcon, this);
    w->setLookAndFeel (&lnf);
    w->addTextEditor ("name", proc.presets.getCurrentName());
    w->addButton ("GUARDAR", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("CANCELAR", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    juce::Component::SafePointer<DesgraciaEditor> safe (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([safe, w] (int result)
    {
        if (safe == nullptr || result != 1) return;
        const auto name = w->getTextEditorContents ("name");
        if (safe->proc.presets.saveUser (name)) { safe->categoryBox.setSelectedId (1, juce::dontSendNotification); safe->character.say ("guardado: " + name, 2.0f); }
        else safe->character.say ("no pude guardar :(", 2.0f);
    }), true);
}
