#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "DesgraciaLookAndFeel.h"
#include "DesgraciaCharacter.h"

namespace ui
{
    using ShowFn = std::function<void (const juce::String& name, const juce::String& value)>;

    // Rotary knob + name; shows the value while hovering/dragging.
    class Knob : public juce::Component
    {
    public:
        Knob (juce::AudioProcessorValueTreeState&, const juce::String& paramId, const juce::String& label, ShowFn);
        void resized() override;
        void paint (juce::Graphics&) override;
        void setLabel (const juce::String& l) { label = l; repaint(); }
        juce::Slider slider;
    private:
        juce::String label;
        juce::RangedAudioParameter* param = nullptr;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
        ShowFn onShow;
    };

    class Choice : public juce::Component
    {
    public:
        Choice (juce::AudioProcessorValueTreeState&, const juce::String& paramId, const juce::String& label = {});
        void resized() override;
        void paint (juce::Graphics&) override;
        juce::ComboBox box;
    private:
        juce::String label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> att;
    };

    class Led : public juce::ToggleButton
    {
    public:
        Led (juce::AudioProcessorValueTreeState&, const juce::String& paramId, const juce::String& text);
    private:
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> att;
    };

    class Section : public juce::Component
    {
    public:
        Section (const juce::String& idx, const juce::String& title) : index (idx), title (title) { setInterceptsMouseClicks (false, true); }
        void paint (juce::Graphics&) override;
        int headerTextWidth() const;
        juce::String index, title;
        bool hideTitle = false;
    };

    class WaveView : public juce::Component
    {
    public:
        void set (int w, float s, float fm, bool showFm) { if (w != wave || s != shape || fm != fmAmt) { wave = w; shape = s; fmAmt = fm; repaint(); } juce::ignoreUnused (showFm); }
        void paint (juce::Graphics&) override;
    private:
        int wave = -1; float shape = -1.0f, fmAmt = 0.0f;
    };

    class EnvView : public juce::Component
    {
    public:
        void set (float a, float d, float s, float r);
        void paint (juce::Graphics&) override;
    private:
        float A = 0, D = 0, S = 0, R = 0;
    };

    class FilterView : public juce::Component
    {
    public:
        void set (int type, float cutoff, float reso);
        void paint (juce::Graphics&) override;
    private:
        int type = -1; float cut = 0, res = 0;
    };

    class LevelMeter : public juce::Component
    {
    public:
        void set (float v) { if (std::abs (v - level) > 0.001f) { level = v; repaint(); } }
        void paint (juce::Graphics&) override;
    private:
        float level = 0.0f;
    };
}

class DesgraciaEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit DesgraciaEditor (DesgraciaProcessor&);
    ~DesgraciaEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void layoutContent();
    void showPresetMenu();
    void savePresetDialog();
    void refreshPresetDisplay();
    void showValue (const juce::String& name, const juce::String& value);

    ui::Knob& knob (const juce::String& id, const juce::String& label);
    ui::Choice& choice (const juce::String& id, const juce::String& label = {});
    ui::Led& led (const juce::String& id, const juce::String& text);
    ui::Section& section (const juce::String& idx, const juce::String& title);

    float param (const juce::String& id) const;

    DesgraciaProcessor& proc;
    DesgraciaLookAndFeel lnf;

    struct Content : public juce::Component { void paint (juce::Graphics&) override; } content;

    juce::OwnedArray<ui::Knob> knobs;
    juce::OwnedArray<ui::Choice> choicesList;
    juce::OwnedArray<ui::Led> leds;
    juce::OwnedArray<ui::Section> sections;
    std::map<juce::String, ui::Knob*> knobMap;
    std::map<juce::String, ui::Choice*> choiceMap;
    std::map<juce::String, ui::Section*> sectionMap;

    ui::WaveView waveA, waveB;
    ui::EnvView ampView, fenvView;
    ui::FilterView filterView;
    ui::LevelMeter meter;
    DesgraciaCharacter character;
    juce::MidiKeyboardComponent keyboard;

    juce::ComboBox categoryBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, nameBtn, randBtn { "RAND" }, saveBtn { "SAVE" }, initBtn { "INIT" };
    juce::Label status;
    juce::String lastShownPreset;
    std::atomic<bool> presetDirty { true };
    int statusHold = 0;

    static constexpr int baseW = 1280, baseH = 800;
};
