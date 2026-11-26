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
        void setLabel (const juce::String& l) { if (l != label) { label = l; repaint(); } }
        void setBig (bool b) { big = b; resized(); }
        juce::Slider slider;
    private:
        juce::String label;
        bool big = false;
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

    // Glass panel with a title; optional one-line help at the bottom.
    class Section : public juce::Component
    {
    public:
        Section (const juce::String& t, const juce::String& h = {}) : title (t), help (h) { setInterceptsMouseClicks (false, true); }
        void paint (juce::Graphics&) override;
        juce::String title, help;
        bool hideTitle = false;
    };

    class WaveView : public juce::Component
    {
    public:
        void set (int w, float s, float fm) { if (w != wave || s != shape || fm != fmAmt) { wave = w; shape = s; fmAmt = fm; repaint(); } }
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

    // A dice you can click. Rolls (animated) when clicked.
    class DiceButton : public juce::Button
    {
    public:
        DiceButton() : juce::Button ("dice") {}
        void paintButton (juce::Graphics&, bool over, bool down) override;
        void roll() { rollFrames = 9; }
        bool tick (juce::Random& r) { if (rollFrames <= 0) return false; --rollFrames; face = 1 + r.nextInt (6); repaint(); return true; }
        juce::Colour pips = pal::navy;
        int face = 5;
    private:
        int rollFrames = 0;
    };

    // Big glossy "RANDOM TOTAL" button with a dice on it.
    class RandomButton : public juce::Button
    {
    public:
        RandomButton() : juce::Button ("RANDOM TOTAL") {}
        void paintButton (juce::Graphics&, bool over, bool down) override;
        int face = 6;
    };

    // One category on the TOCAR page: click = browse/load it, dice = new random sound of that kind.
    class CategoryCard : public juce::Component
    {
    public:
        CategoryCard (const juce::String& cat, const juce::String& desc);
        void paint (juce::Graphics&) override;
        void resized() override;
        void mouseUp (const juce::MouseEvent&) override;
        void mouseEnter (const juce::MouseEvent&) override { repaint(); }
        void mouseExit (const juce::MouseEvent&) override { repaint(); }
        void setActive (bool a) { if (a != active) { active = a; repaint(); } }
        void setCount (int n) { count = n; }

        juce::String category, description;
        DiceButton dice;
        std::function<void()> onPick, onDice;
    private:
        bool active = false;
        int count = 0;
    };

    class VocoderView : public juce::Component
    {
    public:
        explicit VocoderView (Vocoder& v) : voc (v) {}
        void update (bool on, int bands);
        void paint (juce::Graphics&) override;
    private:
        Vocoder& voc;
        float lv[Vocoder::maxBands] {};
        float gate = 0.0f;
        int vowel = 0, nb = 16;
        bool isOn = false;
    };

    // Transparent page holder.
    struct Page : public juce::Component
    {
        Page() { setInterceptsMouseClicks (false, true); }
    };
}

class DesgraciaEditor : public juce::AudioProcessorEditor, private juce::Timer, private juce::ListBoxModel
{
public:
    explicit DesgraciaEditor (DesgraciaProcessor&);
    ~DesgraciaEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void showPage (int index);   // 0 TOCAR, 1 SYNTH, 2 VOCODER, 3 FX

private:
    void timerCallback() override;
    void layoutContent();
    void showPresetMenu();
    void savePresetDialog();
    void refreshPresetDisplay();
    void rebuildList();
    void browse (const juce::String& category, bool loadIfOutside);
    void showValue (const juce::String& name, const juce::String& value);
    void rollVoice();

    // ListBoxModel
    int getNumRows() override { return listRows.size(); }
    void paintListBoxItem (int row, juce::Graphics&, int w, int h, bool selected) override;
    void listBoxItemClicked (int row, const juce::MouseEvent&) override;

    ui::Knob& knob (juce::Component& parent, const juce::String& id, const juce::String& label);
    ui::Choice& choice (juce::Component& parent, const juce::String& id, const juce::String& label = {});
    ui::Led& led (juce::Component& parent, const juce::String& id, const juce::String& text);
    ui::Section& section (juce::Component& parent, const juce::String& key, const juce::String& title, const juce::String& help = {});

    float param (const juce::String& id) const;

    DesgraciaProcessor& proc;
    DesgraciaLookAndFeel lnf;

    struct Content : public juce::Component
    {
        void paint (juce::Graphics&) override;
        juce::Image bgCache;
        juce::Colour accent = pal::aqua;
        juce::String hint;
    } content;

    ui::Page pages[4];
    juce::TextButton tabs[4];
    int currentPage = 0;

    juce::OwnedArray<ui::Knob> knobs;
    juce::OwnedArray<ui::Choice> choicesList;
    juce::OwnedArray<ui::Led> leds;
    juce::OwnedArray<ui::Section> sections;
    juce::OwnedArray<ui::CategoryCard> cards;
    std::map<juce::String, ui::Knob*> knobMap;
    std::map<juce::String, ui::Choice*> choiceMap;
    std::map<juce::String, ui::Led*> ledMap;
    std::map<juce::String, ui::Section*> sectionMap;

    ui::WaveView waveA, waveB;
    ui::EnvView ampView, fenvView;
    ui::FilterView filterView;
    ui::LevelMeter meter;
    ui::VocoderView vocView;
    ui::DiceButton vocDice;
    DesgraciaCharacter character;
    juce::MidiKeyboardComponent keyboard;
    juce::TooltipWindow tooltips { nullptr, 650 };

    juce::ListBox presetList { "presets", this };
    juce::Array<int> listRows;
    juce::String browseCategory { "ALL" };
    juce::TextButton allBtn { "VER TODOS" };

    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, nameBtn, saveBtn { "GUARDAR" }, initBtn { "INIT" };
    ui::RandomButton randBtn;
    juce::Label status;
    juce::String lastShownPreset;
    std::atomic<bool> presetDirty { true };
    int statusHold = 0;
    juce::Random rng;

    static constexpr int baseW = 1280, baseH = 820;
};
