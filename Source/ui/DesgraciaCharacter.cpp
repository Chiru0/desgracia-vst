#include "DesgraciaCharacter.h"
#include "DesgraciaLookAndFeel.h"
#include "PluginProcessor.h"
#include "BinaryData.h"

namespace
{
    constexpr float imgSize = 640.0f;
    constexpr int splitJaw = 395, splitNeck = 500;
    const juce::Rectangle<float> leftEye  { 100.0f, 160.0f, 140.0f, 92.0f };
    const juce::Rectangle<float> rightEye { 372.0f, 186.0f, 138.0f, 96.0f };

    const char* const notePhrases[] = { "ay", "que desgracia", "otra vez?", "mmm", "uh", "eso", "fuaa", "no pares", "jaja", "..." };
    const char* const clickPhrases[] = { "ay!", "no me toques", "que queres", "dejame", "eh!" };
}

DesgraciaCharacter::DesgraciaCharacter (DesgraciaProcessor& p) : proc (p), tint (pal::acid)
{
    face = juce::ImageCache::getFromMemory (BinaryData::desgracia_png, BinaryData::desgracia_pngSize);
    setOpaque (true);
    startTimerHz (60);
}

void DesgraciaCharacter::say (const juce::String& text, float seconds)
{
    bubble = text;
    bubbleTime = seconds;
}

void DesgraciaCharacter::mouseDown (const juce::MouseEvent&)
{
    say (clickPhrases[rng.nextInt (5)], 1.2f);
    bounceVel += 0.9f;
    tiltVel += (rng.nextBool() ? 1.0f : -1.0f) * 1.2f;
    blinkT = 0.0f;
    idleTime = 0.0f;
}

void DesgraciaCharacter::timerCallback()
{
    const float dt = 1.0f / 60.0f;
    time += dt;

    // ---- audio reactive inputs
    const float lv = proc.outputLevel.load (std::memory_order_relaxed);
    level += (lv - level) * (lv > level ? 0.6f : 0.15f);
    const float targetMouth = juce::jlimit (0.0f, 1.0f, level * 4.0f);
    mouth += (targetMouth - mouth) * (targetMouth > mouth ? 0.55f : 0.18f);

    auto raw = [this] (const char* id) { auto* r = proc.apvts.getRawParameterValue (id); return r ? r->load() : 0.0f; };
    drive = raw ("drv_on") > 0.5f ? raw ("drv_amt") * raw ("drv_mix") : 0.0f;
    crush = raw ("crs_on") > 0.5f ? juce::jlimit (0.0f, 1.0f, (12.0f - raw ("crs_bits")) / 10.0f) * raw ("crs_mix") : 0.0f;

    const int nc = proc.engine.noteOnCounter.load();
    if (nc != lastNoteCounter)
    {
        const float vel = proc.engine.lastVelocity.load();
        bounceVel += 0.35f + 0.6f * vel;
        tiltVel += (rng.nextFloat() - 0.5f) * 1.6f * vel;
        lastNoteCounter = nc;
        idleTime = 0.0f;
        if (sleep > 0.5f) { say ("eh? que?", 1.0f); blinkT = 0.0f; }
        else if (rng.nextInt (22) == 0) say (notePhrases[rng.nextInt (10)], 0.9f);
    }

    // ---- springs
    bounceVel += (-bounce * 180.0f - bounceVel * 11.0f) * dt;
    bounce += bounceVel * dt;
    tiltVel += (-tilt * 120.0f - tiltVel * 8.0f) * dt;
    tilt += tiltVel * dt;

    // ---- blink & sleep
    blinkT += dt;
    nextBlink -= dt;
    if (nextBlink <= 0.0f) { blinkT = 0.0f; nextBlink = 1.8f + rng.nextFloat() * 4.5f; }

    const bool silent = proc.engine.activeVoices.load() == 0 && level < 0.002f;
    idleTime = silent ? idleTime + dt : 0.0f;
    sleep += ((idleTime > 9.0f ? 1.0f : 0.0f) - sleep) * 0.03f;

    if (bubbleTime > 0.0f) bubbleTime -= dt;

    // ---- scope snapshot (stable: start on a rising zero crossing)
    const int w = proc.scopeWrite.load (std::memory_order_acquire);
    const int N = DesgraciaProcessor::scopeSize;
    int start = (w - 512 + N) & (N - 1);
    for (int i = 0; i < 256; ++i)
    {
        const int a = (start + i) & (N - 1), b = (start + i + 1) & (N - 1);
        if (proc.scope[a] <= 0.0f && proc.scope[b] > 0.0f) { start = b; break; }
    }
    for (int i = 0; i < 256; ++i) scopeCopy[i] = proc.scope[(start + i) & (N - 1)];

    repaint();
}

void DesgraciaCharacter::drawFace (juce::Graphics& g, juce::Rectangle<float> area)
{
    if (! face.isValid()) return;

    const float s = juce::jmin (area.getWidth() * 0.95f, area.getHeight() * 1.08f) / imgSize;
    const float breathe = std::sin (time * 1.6f) * 0.006f;
    const float sway = std::sin (time * 0.7f) * 0.025f + tilt * 0.12f;
    const float sx = s * (1.0f - bounce * 0.05f), sy = s * (1.0f + bounce * 0.07f + breathe);

    float shakeX = 0.0f, shakeY = 0.0f;
    if (drive > 0.02f)
    {
        const float amt = drive * juce::jlimit (0.0f, 1.0f, level * 5.0f) * 7.0f;
        shakeX = (rng.nextFloat() - 0.5f) * amt;
        shakeY = (rng.nextFloat() - 0.5f) * amt;
    }

    const auto pivot = juce::Point<float> (area.getCentreX() + shakeX, area.getBottom() - area.getHeight() * 0.08f + shakeY);
    const auto base = juce::AffineTransform::translation (-imgSize * 0.5f, -imgSize * 0.92f)
                          .rotated (sway)
                          .scaled (sx, sy)
                          .translated (pivot);

    const float jaw = mouth * 30.0f * (1.0f - sleep * 0.8f);
    const float headLift = -mouth * 5.0f;

    auto drawParts = [&] (juce::Colour c, float dx)
    {
        g.setColour (c);
        const auto off = juce::AffineTransform::translation (dx, 0.0f);
        g.drawImageTransformed (face.getClippedImage ({ 0, 0, 640, splitJaw }),
                                juce::AffineTransform::translation (0, headLift).followedBy (base).followedBy (off), true);
        g.drawImageTransformed (face.getClippedImage ({ 0, splitJaw, 640, splitNeck - splitJaw }),
                                juce::AffineTransform::translation (0, (float) splitJaw + jaw).followedBy (base).followedBy (off), true);
        g.drawImageTransformed (face.getClippedImage ({ 0, splitNeck, 640, 640 - splitNeck }),
                                juce::AffineTransform::translation (0, (float) splitNeck).followedBy (base).followedBy (off), true);
    };

    // mouth cavity: dark gap with a tint glow while singing
    if (jaw > 1.5f)
    {
        juce::Path cav;
        cav.addRoundedRectangle (70.0f, (float) splitJaw - 8.0f + headLift, 330.0f, jaw + 16.0f, 12.0f);
        g.setColour (tint.withAlpha (0.10f + 0.25f * mouth));
        g.fillPath (cav, base);
    }

    const float chroma = drive * 9.0f * juce::jlimit (0.0f, 1.0f, level * 6.0f);
    if (chroma > 0.8f)
    {
        const float d = chroma;
        drawParts (pal::hot.withAlpha (0.55f), d);
        drawParts (pal::cyan.withAlpha (0.45f), -d);
    }
    drawParts (tint, 0.0f);

    // glitch bands
    if ((drive > 0.1f || crush > 0.1f) && level > 0.05f && rng.nextFloat() < 0.12f)
    {
        for (int k = 0; k < 1; ++k)
        {
            const float by = area.getY() + rng.nextFloat() * area.getHeight();
            const float bh = 3.0f + rng.nextFloat() * 14.0f;
            juce::Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (juce::Rectangle<float> (area.getX(), by, area.getWidth(), bh).toNearestInt());
            g.setColour (pal::panel);
            g.fillRect (area);
            drawParts (k == 0 ? tint : pal::hot, (rng.nextFloat() - 0.5f) * 40.0f);
        }
    }

    // eyelids: blink + sleep
    const float blinkAmt = blinkT < 0.16f ? std::sin (blinkT / 0.16f * juce::MathConstants<float>::pi) : 0.0f;
    const float closed = juce::jmax (blinkAmt, sleep * 0.85f);
    if (closed > 0.02f)
    {
        const auto headT = juce::AffineTransform::translation (0, headLift).followedBy (base);
        for (auto eye : { leftEye, rightEye })
        {
            auto lid = eye.withHeight (eye.getHeight() * closed);
            juce::Path p; p.addRectangle (lid.withTrimmedTop (14.0f));
            g.setColour (pal::panel);
            g.fillPath (p, headT);
            juce::Path l;
            l.startNewSubPath (eye.getX() + 6, lid.getBottom());
            l.quadraticTo (eye.getCentreX(), lid.getBottom() + 10.0f * closed, eye.getRight() - 6, lid.getBottom());
            g.setColour (tint);
            g.strokePath (l, juce::PathStrokeType (16.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded), headT);
        }
    }

    if (sleep > 0.3f)
    {
        g.setColour (tint.withAlpha (sleep));
        g.setFont (fonts::mono (14.0f + std::sin (time * 2.0f) * 2.0f, true));
        const float zy = area.getY() + area.getHeight() * 0.18f - std::fmod (time * 8.0f, 20.0f);
        g.drawText ("z Z z", juce::Rectangle<float> (area.getRight() - 90.0f, zy, 80.0f, 20.0f), juce::Justification::centred);
    }
}

void DesgraciaCharacter::drawScope (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (pal::bg);
    g.fillRect (r);
    g.setColour (pal::line);
    g.drawHorizontalLine ((int) r.getCentreY(), r.getX(), r.getRight());
    juce::Path p;
    for (int i = 0; i < 256; ++i)
    {
        const float x = r.getX() + r.getWidth() * (float) i / 255.0f;
        const float y = r.getCentreY() - juce::jlimit (-1.0f, 1.0f, scopeCopy[i] * 1.6f) * r.getHeight() * 0.45f;
        if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
    }
    g.setColour (tint.withAlpha (0.9f));
    g.strokePath (p, juce::PathStrokeType (1.5f));
    g.setColour (pal::dim);
    g.setFont (fonts::mono (9.0f));
    g.drawText ("OUT", r.reduced (5, 3), juce::Justification::topLeft);
}

void DesgraciaCharacter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.fillAll (pal::panel);

    // retro screen grid + scanlines
    g.setColour (pal::line.withAlpha (0.35f));
    for (float x = r.getX(); x < r.getRight(); x += 16.0f) g.drawVerticalLine ((int) x, r.getY(), r.getBottom());
    for (float y = r.getY(); y < r.getBottom(); y += 16.0f) g.drawHorizontalLine ((int) y, r.getX(), r.getRight());

    auto scopeArea = r.removeFromBottom (46.0f).reduced (6.0f, 4.0f);
    auto faceArea = r.reduced (10.0f, 6.0f);

    if (crush > 0.05f)
    {
        const int n = juce::jlimit (40, 320, (int) (320.0f * (1.0f - crush * 0.8f)));
        if (! pixelBuf.isValid() || pixelBuf.getWidth() != n)
            pixelBuf = juce::Image (juce::Image::ARGB, n, n, true);
        pixelBuf.clear (pixelBuf.getBounds());
        {
            juce::Graphics pg (pixelBuf);
            drawFace (pg, pixelBuf.getBounds().toFloat());
        }
        const float side = juce::jmin (faceArea.getWidth(), faceArea.getHeight());
        g.setImageResamplingQuality (juce::Graphics::lowResamplingQuality);
        g.drawImage (pixelBuf, faceArea.withSizeKeepingCentre (side, side));
        g.setImageResamplingQuality (juce::Graphics::mediumResamplingQuality);
    }
    else
        drawFace (g, faceArea);

    // scanline overlay
    g.setColour (juce::Colours::black.withAlpha (0.18f));
    for (float y = r.getY(); y < r.getBottom(); y += 3.0f) g.drawHorizontalLine ((int) y, r.getX(), r.getRight());

    drawScope (g, scopeArea);

    // speech bubble
    if (bubbleTime > 0.0f && bubble.isNotEmpty())
    {
        g.setFont (fonts::mono (13.0f, true));
        const float tw = juce::jmin (r.getWidth() - 20.0f, (float) juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), bubble) + 20.0f);
        juce::Rectangle<float> b (r.getX() + 10.0f, r.getY() + 10.0f, tw, 26.0f);
        const float a = juce::jlimit (0.0f, 1.0f, bubbleTime * 4.0f);
        g.setColour (pal::bg.withAlpha (0.9f * a));
        g.fillRect (b);
        g.setColour (tint.withAlpha (a));
        g.drawRect (b, 1.5f);
        juce::Path tail;
        tail.addTriangle (b.getX() + 24, b.getBottom(), b.getX() + 38, b.getBottom(), b.getX() + 44, b.getBottom() + 10);
        g.fillPath (tail);
        g.drawText (bubble, b.reduced (10, 0), juce::Justification::centredLeft, true);
    }

    g.setColour (pal::line.brighter (0.2f));
    g.drawRect (getLocalBounds(), 1);
}
