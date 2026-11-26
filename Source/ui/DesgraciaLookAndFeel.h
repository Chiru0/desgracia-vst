#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Frutiger Aero / Y2K palette: sky, water, glass, leaves.
namespace pal
{
    const juce::Colour navy     { 0xff0b3556 };
    const juce::Colour navySoft { 0xff3d6687 };
    const juce::Colour aqua     { 0xff19a7ff };
    const juce::Colour aquaDeep { 0xff0a6fd0 };
    const juce::Colour leaf     { 0xff4cc93a };
    const juce::Colour sky      { 0xff63c7ff };
    const juce::Colour glass    { 0xffffffff };
    const juce::Colour hot      { 0xffff5a36 };
    const juce::Colour cyan     { 0xff3fe0ff };

    // legacy names used by some widgets
    const juce::Colour bg    = navy;
    const juce::Colour panel = juce::Colour (0xffeaf7ff);
    const juce::Colour line  = juce::Colour (0xffb7dcf2);
    const juce::Colour text  = navy;
    const juce::Colour dim   = navySoft;
    const juce::Colour acid  = aqua;

    juce::Colour forCategory (const juce::String& category);
}

namespace fonts
{
    juce::Font sans (float size, bool bold = false);
    juce::Font mono (float size, bool bold = false);
}

namespace aero
{
    // glossy glass panel (rounded) with optional coloured header pill
    void glassPanel (juce::Graphics&, juce::Rectangle<float> r, float radius = 14.0f, float alpha = 1.0f);
    void glossyPill (juce::Graphics&, juce::Rectangle<float> r, juce::Colour base, bool pressed = false, bool hover = false);
    void orb (juce::Graphics&, juce::Rectangle<float> r, juce::Colour base, float gloss = 1.0f);
    void bubble (juce::Graphics&, juce::Point<float> c, float radius, float alpha = 1.0f);
    void drawDice (juce::Graphics&, juce::Rectangle<float> r, int face, juce::Colour pips);
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
    juce::Font getComboBoxFont (juce::ComboBox&) override { return fonts::sans (11.5f, true); }
    juce::Font getPopupMenuFont() override { return fonts::sans (13.5f); }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return fonts::sans (12.0f, true); }
    juce::Font getLabelFont (juce::Label&) override { return fonts::sans (12.0f); }
    void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;
    void drawPopupMenuItem (juce::Graphics&, const juce::Rectangle<int>& area, bool isSeparator, bool isActive, bool isHighlighted,
                            bool isTicked, bool hasSubMenu, const juce::String& text, const juce::String& shortcutKeyText,
                            const juce::Drawable* icon, const juce::Colour* textColour) override;
    void drawPopupMenuSectionHeader (juce::Graphics&, const juce::Rectangle<int>& area, const juce::String& sectionName) override;
    void drawTooltip (juce::Graphics&, const juce::String& text, int width, int height) override;
    juce::Rectangle<int> getTooltipBounds (const juce::String& tipText, juce::Point<int> screenPos, juce::Rectangle<int> parentArea) override;

private:
    juce::Colour accent = pal::aqua;
};
