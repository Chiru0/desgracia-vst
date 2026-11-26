#include "DesgraciaLookAndFeel.h"

juce::Colour pal::forCategory (const juce::String& c)
{
    if (c == "ICONIC LEADS")  return acid;
    if (c == "WEIRD PLUCKS")  return cyan;
    if (c == "DIGITAL KEYS")  return juce::Colour (0xffffd23a);
    if (c == "METALLIC / FM") return juce::Colour (0xffc3ccd6);
    if (c == "DARK TEXTURES") return juce::Colour (0xffa47bff);
    if (c == "AMBIENT")       return juce::Colour (0xff7fffd0);
    if (c == "BASS")          return hot;
    if (c == "EXPERIMENTAL")  return juce::Colour (0xffff4fd8);
    return juce::Colour (0xfff2f2ea);
}

juce::Font fonts::mono (float size, bool bold)
{
    return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), size,
                                          bold ? juce::Font::bold : juce::Font::plain));
}

DesgraciaLookAndFeel::DesgraciaLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, pal::panel2);
    setColour (juce::ComboBox::textColourId, pal::text);
    setColour (juce::ComboBox::outlineColourId, pal::line);
    setColour (juce::ComboBox::arrowColourId, pal::dim);
    setColour (juce::PopupMenu::backgroundColourId, pal::panel);
    setColour (juce::PopupMenu::textColourId, pal::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, pal::acid);
    setColour (juce::PopupMenu::highlightedTextColourId, pal::bg);
    setColour (juce::TextButton::buttonColourId, pal::panel2);
    setColour (juce::TextButton::textColourOffId, pal::text);
    setColour (juce::Label::textColourId, pal::text);
    setColour (juce::TextEditor::backgroundColourId, pal::panel2);
    setColour (juce::TextEditor::textColourId, pal::text);
    setColour (juce::TextEditor::outlineColourId, pal::line);
    setColour (juce::TextEditor::focusedOutlineColourId, pal::acid);
    setColour (juce::AlertWindow::backgroundColourId, pal::panel);
    setColour (juce::AlertWindow::textColourId, pal::text);
    setColour (juce::AlertWindow::outlineColourId, pal::acid);
    setColour (juce::ResizableWindow::backgroundColourId, pal::bg);
}

void DesgraciaLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                             float start, float end, juce::Slider& s)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.0f);
    const float size = juce::jmin (b.getWidth(), b.getHeight());
    b = b.withSizeKeepingCentre (size, size);
    const auto c = b.getCentre();
    const float r = size * 0.5f;
    const bool active = s.isMouseOverOrDragging();
    const bool bip = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;

    // tick ring (retro hardware scale)
    for (int i = 0; i <= 10; ++i)
    {
        const float a = start + (end - start) * (float) i / 10.0f;
        const float r0 = r - 1.0f, r1 = r - (i % 5 == 0 ? 5.0f : 3.0f);
        g.setColour (pal::line.brighter (0.25f));
        g.drawLine (c.x + r0 * std::sin (a), c.y - r0 * std::cos (a), c.x + r1 * std::sin (a), c.y - r1 * std::cos (a), 1.0f);
    }

    // value arc
    const float ang = start + pos * (end - start);
    const float from = bip ? start + 0.5f * (end - start) : start;
    juce::Path arc;
    arc.addCentredArc (c.x, c.y, r - 7.0f, r - 7.0f, 0.0f, juce::jmin (from, ang), juce::jmax (from, ang), true);
    g.setColour (pal::line);
    juce::Path back; back.addCentredArc (c.x, c.y, r - 7.0f, r - 7.0f, 0.0f, start, end, true);
    g.strokePath (back, juce::PathStrokeType (2.5f));
    g.setColour (active ? accent : accent.withMultipliedSaturation (0.85f).withMultipliedBrightness (0.85f));
    g.strokePath (arc, juce::PathStrokeType (2.5f, juce::PathStrokeType::mitered, juce::PathStrokeType::butt));

    // body: flat square-ish cap, industrial
    const float br = r - 11.0f;
    g.setColour (pal::panel2.brighter (active ? 0.12f : 0.05f));
    g.fillEllipse (c.x - br, c.y - br, br * 2, br * 2);
    g.setColour (pal::line.brighter (0.4f));
    g.drawEllipse (c.x - br, c.y - br, br * 2, br * 2, 1.0f);

    // pointer
    g.setColour (pal::text);
    juce::Path ptr;
    ptr.addRectangle (-1.25f, -br, 2.5f, br * 0.6f);
    g.fillPath (ptr, juce::AffineTransform::rotation (ang).translated (c.x, c.y));
}

void DesgraciaLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    auto r = b.getLocalBounds().toFloat();
    const bool on = b.getToggleState();
    auto led = r.removeFromLeft (r.getHeight()).reduced (r.getHeight() * 0.28f);
    g.setColour (on ? accent : pal::line.brighter (0.2f));
    g.fillRect (led);
    if (on)
    {
        g.setColour (accent.withAlpha (0.25f));
        g.fillRect (led.expanded (2.5f));
    }
    g.setColour (on ? pal::text : (highlighted ? pal::text : pal::dim));
    g.setFont (fonts::mono (10.5f, true));
    g.drawText (b.getButtonText(), r.withTrimmedLeft (2), juce::Justification::centredLeft, false);
}

void DesgraciaLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hi, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (down ? accent : (hi ? pal::panel2.brighter (0.15f) : pal::panel2));
    g.fillRect (r);
    g.setColour (hi ? accent.withAlpha (0.8f) : pal::line.brighter (0.15f));
    g.drawRect (r, 1.0f);
}

void DesgraciaLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool down)
{
    g.setFont (fonts::mono (11.0f, true));
    g.setColour (down ? pal::bg : pal::text);
    g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred, false);
}

void DesgraciaLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (pal::panel2);
    g.fillRect (r);
    g.setColour (box.isMouseOver (true) ? accent.withAlpha (0.8f) : pal::line.brighter (0.15f));
    g.drawRect (r, 1.0f);
    // little triangle
    juce::Path t;
    const float ax = (float) w - 9.0f, ay = (float) h * 0.5f;
    t.addTriangle (ax - 3.5f, ay - 2.0f, ax + 3.5f, ay - 2.0f, ax, ay + 2.5f);
    g.setColour (pal::dim);
    g.fillPath (t);
}

void DesgraciaLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (4, 0, box.getWidth() - 16, box.getHeight());
    label.setFont (getComboBoxFont (box));
    label.setJustificationType (juce::Justification::centredLeft);
}

void DesgraciaLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.fillAll (pal::panel);
    g.setColour (accent.withAlpha (0.6f));
    g.drawRect (0, 0, w, h, 1);
}

void DesgraciaLookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area, bool isSeparator, bool isActive,
                                              bool isHighlighted, bool isTicked, bool hasSubMenu, const juce::String& text,
                                              const juce::String&, const juce::Drawable*, const juce::Colour*)
{
    if (isSeparator)
    {
        g.setColour (pal::line);
        g.fillRect (area.reduced (6, area.getHeight() / 2).withHeight (1));
        return;
    }
    auto r = area.reduced (1, 0);
    if (isHighlighted && isActive) { g.setColour (accent); g.fillRect (r); }
    g.setColour (isHighlighted ? pal::bg : (isActive ? pal::text : pal::dim));
    g.setFont (fonts::mono (13.0f, isTicked));
    auto tr = r.reduced (10, 0);
    if (isTicked)
    {
        g.setColour (isHighlighted ? pal::bg : accent);
        g.drawText (">", tr.removeFromLeft (12), juce::Justification::centredLeft);
        g.setColour (isHighlighted ? pal::bg : pal::text);
    }
    else tr.removeFromLeft (12);
    g.drawText (text, tr, juce::Justification::centredLeft, true);
    if (hasSubMenu)
        g.drawText (">>", r.reduced (8, 0), juce::Justification::centredRight);
}

void DesgraciaLookAndFeel::drawPopupMenuSectionHeader (juce::Graphics& g, const juce::Rectangle<int>& area, const juce::String& name)
{
    g.setColour (pal::forCategory (name));
    g.setFont (fonts::mono (11.0f, true));
    g.drawText ("// " + name, area.reduced (8, 0), juce::Justification::bottomLeft);
}
