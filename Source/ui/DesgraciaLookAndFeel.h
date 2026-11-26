#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace pal
{
    const juce::Colour bg      { 0xff0a0a0b };
    const juce::Colour panel   { 0xff131315 };
    const juce::Colour panel2  { 0xff1a1a1d };
    const juce::Colour line    { 0xff2a2a2e };
    const juce::Colour text    { 0xffd2d2c6 };
    const juce::Colour dim     { 0xff6c6c66 };
    const juce::Colour acid    { 0xffd7ff3a };
    const juce::Colour hot     { 0xffff4d2e };
    const juce::Colour cyan    { 0xff4de1ff };

    juce::Colour forCategory (const juce::String& category);
}

namespace fonts
{
    juce::Font mono (float size, bool bold = false);
}

class DesgraciaLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DesgraciaLookAndFeel();

    void setAccent (juce::Colour c) { accent = c; }
    juce::Colour getAccent() const { return accent; }

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool highlighted, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return fonts::mono (11.0f, true); }
    juce::Font getPopupMenuFont() override { return fonts::mono (13.0f); }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return fonts::mono (11.0f, true); }
    juce::Font getLabelFont (juce::Label&) override { return fonts::mono (12.0f); }
    void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;
    void drawPopupMenuItem (juce::Graphics&, const juce::Rectangle<int>& area, bool isSeparator, bool isActive, bool isHighlighted,
                            bool isTicked, bool hasSubMenu, const juce::String& text, const juce::String& shortcutKeyText,
                            const juce::Drawable* icon, const juce::Colour* textColour) override;
    void drawPopupMenuSectionHeader (juce::Graphics&, const juce::Rectangle<int>& area, const juce::String& sectionName) override;

private:
    juce::Colour accent = pal::acid;
};
