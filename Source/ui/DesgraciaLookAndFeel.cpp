#include "DesgraciaLookAndFeel.h"

juce::Colour pal::forCategory (const juce::String& c)
{
    if (c == "ICONIC LEADS")  return juce::Colour (0xff0f9bff);
    if (c == "SYNTHS")        return juce::Colour (0xff6cc417);
    if (c == "WEIRD PLUCKS")  return juce::Colour (0xff1fc46b);
    if (c == "DIGITAL KEYS")  return juce::Colour (0xffffa31a);
    if (c == "METALLIC / FM") return juce::Colour (0xff7d97b8);
    if (c == "VOCODER")       return juce::Colour (0xff00bfae);
    if (c == "CRANK")         return juce::Colour (0xffc23bff);
    if (c == "DARK TEXTURES") return juce::Colour (0xff6c5ce7);
    if (c == "AMBIENT")       return juce::Colour (0xff35c9e8);
    if (c == "BASS")          return juce::Colour (0xffff5a36);
    if (c == "EXPERIMENTAL")  return juce::Colour (0xffff4fc8);
    if (c == "USER")          return juce::Colour (0xff3d6687);
    return aqua;
}

juce::Font fonts::sans (float size, bool bold)
{
    return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
}

juce::Font fonts::mono (float size, bool bold) { return sans (size, bold); }

//============================================================================== aero primitives
void aero::glassPanel (juce::Graphics& g, juce::Rectangle<float> r, float radius, float alpha)
{
    // soft shadow
    for (int i = 3; i > 0; --i)
    {
        g.setColour (juce::Colour (0xff0b3556).withAlpha (0.035f * alpha));
        g.fillRoundedRectangle (r.translated (0, (float) i * 1.5f).expanded ((float) i), radius + (float) i);
    }
    juce::ColourGradient body (juce::Colours::white.withAlpha (0.78f * alpha), r.getX(), r.getY(),
                               juce::Colour (0xffd9f1ff).withAlpha (0.62f * alpha), r.getX(), r.getBottom(), false);
    g.setGradientFill (body);
    g.fillRoundedRectangle (r, radius);
    // top gloss
    auto top = r.reduced (2.0f).withHeight (juce::jmin (r.getHeight() * 0.45f, 60.0f));
    juce::ColourGradient gl (juce::Colours::white.withAlpha (0.75f * alpha), top.getX(), top.getY(),
                             juce::Colours::white.withAlpha (0.0f), top.getX(), top.getBottom(), false);
    g.setGradientFill (gl);
    g.fillRoundedRectangle (top, radius - 2.0f);
    g.setColour (juce::Colours::white.withAlpha (0.95f * alpha));
    g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.3f);
    g.setColour (juce::Colour (0xff7cc4ec).withAlpha (0.5f * alpha));
    g.drawRoundedRectangle (r.expanded (0.5f), radius + 0.5f, 0.8f);
}

void aero::glossyPill (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour base, bool pressed, bool hover)
{
    const float rad = juce::jmin (r.getHeight() * 0.5f, 12.0f);
    auto b = hover ? base.brighter (0.15f) : base;
    juce::ColourGradient grad (b.brighter (pressed ? 0.1f : 0.55f), r.getX(), r.getY(), b.darker (pressed ? 0.1f : 0.25f), r.getX(), r.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (r, rad);
    auto top = r.reduced (2.0f, 1.5f).withHeight (r.getHeight() * 0.48f);
    juce::ColourGradient gl (juce::Colours::white.withAlpha (pressed ? 0.25f : 0.7f), top.getX(), top.getY(),
                             juce::Colours::white.withAlpha (0.08f), top.getX(), top.getBottom(), false);
    g.setGradientFill (gl);
    g.fillRoundedRectangle (top, rad - 1.5f);
    g.setColour (b.darker (0.45f).withAlpha (0.7f));
    g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
}

void aero::orb (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour base, float gloss)
{
    const auto c = r.getCentre();
    const float rad = r.getWidth() * 0.5f;
    juce::ColourGradient grad (base.brighter (0.9f), c.x - rad * 0.3f, c.y - rad * 0.4f, base.darker (0.35f), c.x + rad * 0.6f, c.y + rad, true);
    g.setGradientFill (grad);
    g.fillEllipse (r);
    // inner light from the bottom (water)
    juce::ColourGradient low (base.brighter (0.6f).withAlpha (0.0f), c.x, c.y, base.brighter (0.8f).withAlpha (0.55f), c.x, r.getBottom(), false);
    g.setGradientFill (low);
    g.fillEllipse (r.reduced (rad * 0.12f));
    // specular cap
    auto cap = juce::Rectangle<float> (r.getX() + rad * 0.32f, r.getY() + rad * 0.08f, rad * 1.36f, rad * 0.85f);
    juce::ColourGradient sp (juce::Colours::white.withAlpha (0.9f * gloss), cap.getCentreX(), cap.getY(),
                             juce::Colours::white.withAlpha (0.0f), cap.getCentreX(), cap.getBottom(), false);
    g.setGradientFill (sp);
    g.fillEllipse (cap);
    g.setColour (base.darker (0.6f).withAlpha (0.55f));
    g.drawEllipse (r.reduced (0.5f), 1.0f);
}

void aero::bubble (juce::Graphics& g, juce::Point<float> c, float rad, float a)
{
    juce::ColourGradient grad (juce::Colours::white.withAlpha (0.05f * a), c.x, c.y, juce::Colours::white.withAlpha (0.55f * a), c.x + rad, c.y, true);
    g.setGradientFill (grad);
    g.fillEllipse (c.x - rad, c.y - rad, rad * 2, rad * 2);
    g.setColour (juce::Colours::white.withAlpha (0.8f * a));
    g.drawEllipse (c.x - rad, c.y - rad, rad * 2, rad * 2, juce::jmax (0.6f, rad * 0.06f));
    g.setColour (juce::Colours::white.withAlpha (0.9f * a));
    g.fillEllipse (c.x - rad * 0.55f, c.y - rad * 0.62f, rad * 0.5f, rad * 0.32f);
}

void aero::drawDice (juce::Graphics& g, juce::Rectangle<float> r, int face, juce::Colour pips)
{
    const float rad = r.getWidth() * 0.22f;
    juce::ColourGradient grad (juce::Colours::white, r.getX(), r.getY(), juce::Colour (0xffcfe9fb), r.getRight(), r.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (r, rad);
    auto top = r.reduced (2.0f).withHeight (r.getHeight() * 0.45f);
    g.setColour (juce::Colours::white.withAlpha (0.8f));
    g.fillRoundedRectangle (top, rad * 0.8f);
    g.setColour (pal::navySoft.withAlpha (0.6f));
    g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
    const float s = r.getWidth();
    const float pr = s * 0.09f;
    auto pip = [&] (float fx, float fy) { g.setColour (pips); g.fillEllipse (r.getX() + fx * s - pr, r.getY() + fy * s - pr, pr * 2, pr * 2); };
    const float a = 0.28f, m = 0.5f, b = 0.72f;
    switch (juce::jlimit (1, 6, face))
    {
        case 1: pip (m, m); break;
        case 2: pip (a, a); pip (b, b); break;
        case 3: pip (a, a); pip (m, m); pip (b, b); break;
        case 4: pip (a, a); pip (b, a); pip (a, b); pip (b, b); break;
        case 5: pip (a, a); pip (b, a); pip (m, m); pip (a, b); pip (b, b); break;
        default: pip (a, a); pip (b, a); pip (a, m); pip (b, m); pip (a, b); pip (b, b); break;
    }
}

//============================================================================== look and feel
DesgraciaLookAndFeel::DesgraciaLookAndFeel()
{
    setColour (juce::ComboBox::textColourId, pal::navy);
    setColour (juce::ComboBox::backgroundColourId, juce::Colours::white);
    setColour (juce::ComboBox::outlineColourId, pal::line);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xfff4fbff));
    setColour (juce::PopupMenu::textColourId, pal::navy);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, pal::aqua);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::TextButton::buttonColourId, pal::aqua);
    setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    setColour (juce::Label::textColourId, pal::navy);
    setColour (juce::TextEditor::backgroundColourId, juce::Colours::white);
    setColour (juce::TextEditor::textColourId, pal::navy);
    setColour (juce::TextEditor::outlineColourId, pal::line);
    setColour (juce::TextEditor::focusedOutlineColourId, pal::aqua);
    setColour (juce::AlertWindow::backgroundColourId, juce::Colour (0xffeaf7ff));
    setColour (juce::AlertWindow::textColourId, pal::navy);
    setColour (juce::AlertWindow::outlineColourId, pal::aqua);
    setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::ResizableWindow::backgroundColourId, pal::sky);
    setColour (juce::ScrollBar::thumbColourId, pal::aqua.withAlpha (0.6f));
}

void DesgraciaLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                             float start, float end, juce::Slider& s)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.0f);
    const float size = juce::jmin (b.getWidth(), b.getHeight());
    b = b.withSizeKeepingCentre (size, size);
    const auto c = b.getCentre();
    const float r = size * 0.5f;
    const bool active = s.isMouseOverOrDragging();
    const bool bip = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
    const auto acc = s.isEnabled() ? accent : juce::Colour (0xff9fb6c8);

    // track
    juce::Path back; back.addCentredArc (c.x, c.y, r - 3.0f, r - 3.0f, 0.0f, start, end, true);
    g.setColour (juce::Colours::white.withAlpha (0.85f));
    g.strokePath (back, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (juce::Colour (0xff9cc9e6).withAlpha (0.6f));
    g.strokePath (back, juce::PathStrokeType (1.0f));

    const float ang = start + pos * (end - start);
    const float from = bip ? start + 0.5f * (end - start) : start;
    juce::Path arc;
    arc.addCentredArc (c.x, c.y, r - 3.0f, r - 3.0f, 0.0f, juce::jmin (from, ang), juce::jmax (from, ang), true);
    g.setColour (active ? acc.brighter (0.15f) : acc);
    g.strokePath (arc, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // glossy orb
    const float br = r - 8.0f;
    aero::orb (g, { c.x - br, c.y - br, br * 2, br * 2 }, active ? acc.brighter (0.1f) : acc);

    // pointer
    juce::Path ptr;
    ptr.addRoundedRectangle (-1.6f, -br + 2.0f, 3.2f, br * 0.55f, 1.6f);
    g.setColour (juce::Colours::white);
    g.fillPath (ptr, juce::AffineTransform::rotation (ang).translated (c.x, c.y));
}

void DesgraciaLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    auto r = b.getLocalBounds().toFloat();
    const bool on = b.getToggleState();
    auto led = r.removeFromLeft (r.getHeight()).reduced (r.getHeight() * 0.14f);
    aero::orb (g, led, on ? pal::leaf : juce::Colour (0xffb9c9d6), on ? 1.0f : 0.7f);
    g.setColour (on ? pal::navy : (highlighted ? pal::navy : pal::navySoft));
    g.setFont (fonts::sans (11.5f, true));
    g.drawText (b.getButtonText(), r.withTrimmedLeft (4), juce::Justification::centredLeft, false);
}

void DesgraciaLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hi, bool down)
{
    auto base = b.findColour (juce::TextButton::buttonColourId);
    if (b.getToggleState()) base = accent;
    aero::glossyPill (g, b.getLocalBounds().toFloat().reduced (0.5f), base, down, hi);
}

void DesgraciaLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    g.setFont (fonts::sans (12.0f, true));
    g.setColour (juce::Colours::black.withAlpha (0.18f));
    g.drawText (b.getButtonText(), b.getLocalBounds().translated (0, 1), juce::Justification::centred, false);
    g.setColour (b.findColour (juce::TextButton::textColourOffId));
    g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred, false);
}

void DesgraciaLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    const float rad = juce::jmin (r.getHeight() * 0.5f, 9.0f);
    juce::ColourGradient grad (juce::Colours::white, 0, 0, juce::Colour (0xffdff2ff), 0, (float) h, false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (r, rad);
    g.setColour (box.isMouseOver (true) ? accent : juce::Colour (0xff8fc6ea));
    g.drawRoundedRectangle (r, rad, 1.0f);
    juce::Path t;
    const float ax = (float) w - 10.0f, ay = (float) h * 0.5f;
    t.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (accent);
    g.fillPath (t);
}

void DesgraciaLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (6, 0, box.getWidth() - 18, box.getHeight());
    label.setFont (getComboBoxFont (box));
    label.setJustificationType (juce::Justification::centredLeft);
}

void DesgraciaLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    juce::ColourGradient grad (juce::Colours::white, 0, 0, juce::Colour (0xffe3f4ff), 0, (float) h, false);
    g.setGradientFill (grad);
    g.fillAll();
    g.setColour (accent);
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
    auto r = area.reduced (2, 1).toFloat();
    if (isHighlighted && isActive) aero::glossyPill (g, r, accent);
    g.setColour (isHighlighted ? juce::Colours::white : (isActive ? pal::navy : pal::navySoft));
    g.setFont (fonts::sans (13.5f, isTicked));
    auto tr = area.reduced (12, 0);
    if (isTicked) g.drawText (juce::String::charToString (0x2022), tr.removeFromLeft (12), juce::Justification::centredLeft);
    else tr.removeFromLeft (12);
    g.drawText (text, tr, juce::Justification::centredLeft, true);
    if (hasSubMenu) g.drawText (">", area.reduced (8, 0), juce::Justification::centredRight);
}

void DesgraciaLookAndFeel::drawPopupMenuSectionHeader (juce::Graphics& g, const juce::Rectangle<int>& area, const juce::String& name)
{
    g.setColour (pal::forCategory (name));
    g.setFont (fonts::sans (12.0f, true));
    g.drawText (name, area.reduced (10, 0), juce::Justification::bottomLeft);
}

void DesgraciaLookAndFeel::drawTooltip (juce::Graphics& g, const juce::String& text, int w, int h)
{
    aero::glassPanel (g, juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (1.0f), 8.0f);
    g.setColour (pal::navy);
    g.setFont (fonts::sans (12.5f));
    g.drawFittedText (text, juce::Rectangle<int> (0, 0, w, h).reduced (10, 6), juce::Justification::centredLeft, 4);
}

juce::Rectangle<int> DesgraciaLookAndFeel::getTooltipBounds (const juce::String& tipText, juce::Point<int> pos, juce::Rectangle<int> parent)
{
    const int w = juce::jmin (320, juce::GlyphArrangement::getStringWidthInt (fonts::sans (12.5f), tipText) + 24);
    const int lines = juce::jmax (1, (juce::GlyphArrangement::getStringWidthInt (fonts::sans (12.5f), tipText) + 24) / 300 + 1);
    const int h = 14 + lines * 16;
    return juce::Rectangle<int> (pos.x + 14, pos.y + 18, w, h).constrainedWithin (parent);
}
