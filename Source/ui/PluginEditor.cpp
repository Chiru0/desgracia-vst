#include "PluginEditor.h"
#include "engine/DSPCore.h"

using namespace ui;

namespace
{
    juce::Colour accentOf (juce::Component& c)
    {
        if (auto* l = dynamic_cast<DesgraciaLookAndFeel*> (&c.getLookAndFeel())) return l->getAccent();
        return pal::aqua;
    }

    void inset (juce::Graphics& g, juce::Rectangle<float> r, float rad = 8.0f)
    {
        juce::ColourGradient grad (juce::Colour (0xffeef8ff), r.getX(), r.getY(), juce::Colours::white, r.getX(), r.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (r, rad);
        g.setColour (juce::Colour (0xff9cc9e6));
        g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
    }

    void glowStroke (juce::Graphics& g, const juce::Path& p, juce::Colour c, float w = 2.0f)
    {
        g.setColour (c.withAlpha (0.18f));
        g.strokePath (p, juce::PathStrokeType (w * 3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (c);
        g.strokePath (p, juce::PathStrokeType (w, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    struct CatInfo { const char* name; const char* desc; };
    const CatInfo catInfo[] = {
        { "ICONIC LEADS",  "leads que cortan, los de siempre" },
        { "SYNTHS",        "synths de juguete, feria y arcade" },
        { "WEIRD PLUCKS",  "plucks raros, pixelados, rotos" },
        { "DIGITAL KEYS",  "teclados baratos y marimbas" },
        { "METALLIC / FM", "campanas, latas y FM" },
        { "VOCODER",       "robots que hablan: wa-wa-wa" },
        { "CRANK",         "pulsantes y raros, para crank" },
        { "DARK TEXTURES", "oscuro, sucio, roto" },
        { "AMBIENT",       "pads, nubes y agua" },
        { "BASS",          "bajos piolas, 808 y synth" },
        { "EXPERIMENTAL",  "cualquier cosa, sin reglas" } };

    // Spanish help shown as tooltips. Keep them short and concrete.
    juce::String helpFor (const juce::String& id)
    {
        static const std::map<juce::String, juce::String> h = {
            { "oscA_wave", "Forma de onda del oscilador A. TOY y METAL son los mas sucios." },
            { "oscB_wave", "Forma de onda del oscilador B (tambien modula al A con FM)." },
            { "oscA_oct", "Octava del oscilador A." }, { "oscB_oct", "Octava del oscilador B." },
            { "oscA_semi", "Semitonos: subi 7 para una quinta, 12 para una octava." }, { "oscB_semi", "Semitonos: subi 7 para una quinta, 12 para una octava." },
            { "oscA_fine", "Desafinado fino del A. Poquito = mas gordo." }, { "oscB_fine", "Desafinado fino del B. Poquito = mas gordo." },
            { "oscA_shape", "Cambia el caracter de la onda (que hace depende de la onda)." }, { "oscB_shape", "Cambia el caracter de la onda (que hace depende de la onda)." },
            { "oscA_level", "Volumen del oscilador A." }, { "oscB_level", "Volumen del oscilador B." },
            { "fm_amt", "FM: el B modula al A. Poco = brillo, mucho = metal y campanas." },
            { "fm_env", "FM que sigue a la envolvente de filtro: golpe metalico al principio de la nota." },
            { "ring_amt", "Ring mod A x B: sonidos de lata y robot." },
            { "penv_amt", "Envolvente de pitch: la nota cae o sube al empezar (kick, laser, pluck)." },
            { "penv_decay", "Cuanto tarda la caida de pitch." },
            { "penv_dest", "A que osciladores afecta la envolvente de pitch." },
            { "sub_level", "Sub oscilador: peso abajo, para bajos." },
            { "sub_wave", "Forma del sub." }, { "sub_oct", "Cuantas octavas abajo va el sub." },
            { "noise_level", "Ruido: aire, soplido, textura." }, { "noise_color", "Color del ruido: oscuro a brillante." },
            { "uni_voices", "Unison: cuantas copias de cada nota. Mas = mas ancho." },
            { "uni_detune", "Cuanto se desafinan las copias del unison." },
            { "uni_spread", "Cuanto se abren las copias en estereo." },
            { "drift", "Desafinado analogico/cassette por nota. Lo-fi." },
            { "flt_type", "Tipo de filtro. VOWEL hace que el synth diga vocales." },
            { "flt_cutoff", "Brillo: donde corta el filtro." }, { "flt_reso", "Resonancia: pico chillon en el corte." },
            { "flt_drive", "Saturacion dentro del filtro." },
            { "flt_env", "Cuanto abre el filtro la envolvente de filtro (el 'wow' del ataque)." },
            { "flt_key", "El filtro sigue las notas: agudas mas brillantes." },
            { "flt_vel", "Tocar mas fuerte = mas brillante." },
            { "voice_mode", "POLY = acordes. MONO = una nota. LEGATO = una nota sin reatacar (con glide)." },
            { "glide", "Portamento: la nota se desliza a la siguiente." },
            { "bend_range", "Rango del pitch bend en semitonos." },
            { "width", "Ancho estereo. 0 = mono (ideal para bajos)." },
            { "master", "Volumen final del preset." },
            { "amp_a", "Ataque: cuanto tarda en sonar." }, { "amp_d", "Decay: cuanto tarda en bajar al sustain." },
            { "amp_s", "Sustain: volumen mientras mantenes la tecla. 0 = pluck." },
            { "amp_r", "Release: cola cuando soltas la tecla." }, { "amp_vel", "Cuanto afecta la velocidad al volumen." },
            { "fenv_a", "Ataque de la envolvente de filtro (y FM ENV)." }, { "fenv_d", "Decay de la envolvente de filtro." },
            { "fenv_s", "Sustain de la envolvente de filtro." }, { "fenv_r", "Release de la envolvente de filtro." },
            { "drv_on", "Prende la saturacion." }, { "drv_type", "SOFT suave, HARD rompe, FOLD pliega (raro), TUBE calido." },
            { "drv_amt", "Cuanta mugre." }, { "drv_mix", "Mezcla entre limpio y saturado." },
            { "crs_on", "Prende el bitcrusher (sonido de consola vieja)." }, { "crs_bits", "Bits: menos = mas roto." },
            { "crs_rate", "Baja la frecuencia de muestreo: aliasing y metal." }, { "crs_mix", "Mezcla del crush." },
            { "eq_on", "Prende el ecualizador." }, { "eq_low", "Graves." }, { "eq_mid", "Medios." }, { "eq_midf", "Frecuencia de los medios." }, { "eq_high", "Agudos." },
            { "cho_on", "Prende el chorus." }, { "cho_rate", "Velocidad del chorus." }, { "cho_depth", "Profundidad del chorus." }, { "cho_mix", "Mezcla del chorus." },
            { "dly_on", "Prende el delay (eco)." }, { "dly_sync", "Delay en tiempo con el tempo del DAW." },
            { "dly_div", "Division del delay (1/8, 1/4...)." }, { "dly_time", "Tiempo del delay en ms (si no esta en sync)." },
            { "dly_fb", "Cuantas repeticiones." }, { "dly_tone", "Repeticiones oscuras o brillantes." },
            { "dly_ping", "Ping-pong: el eco salta de lado a lado." }, { "dly_mix", "Cuanto eco." },
            { "rev_on", "Prende la reverb." }, { "rev_size", "Tamano de la sala." }, { "rev_damp", "Que tan apagada suena la reverb." }, { "rev_mix", "Cuanta reverb." },
            { "voc_on", "Prende el vocoder: el synth habla." },
            { "voc_pattern", "Que dice: WA-WA-WA, YOI-YOI, RUIDOS RANDOM..." },
            { "voc_rate", "Cada cuanto cambia de silaba (sincronizado al tempo)." },
            { "voc_bands", "Bandas: pocas = robot pobre, muchas = mas claro." },
            { "voc_shift", "Corre los formantes: voz de nene (+) o de monstruo (-)." },
            { "voc_q", "Resonancia de las bandas: mas = mas metalico." },
            { "voc_speed", "Que tan rapido reacciona a la voz." },
            { "voc_sib", "Agrega las S y T (consonantes)." }, { "voc_mix", "Mezcla entre el synth normal y el vocoder." },
            { "mac_bright", "BRILLO: abre o cierra todo el sonido." },
            { "mac_dirt", "MUGRE: satura, rompe y crushea." },
            { "mac_detune", "DESAFINE: lo vuelve cassette viejo y ancho." },
            { "mac_move", "MOVIMIENTO: agrega vida con los LFOs." },
            { "mac_space", "ESPACIO: reverb y delay en una perilla." },
            { "mac_length", "LARGO: notas mas cortas (izq) o mas largas (der)." } };
        if (auto it = h.find (id); it != h.end()) return it->second;
        if (id.startsWith ("lfo"))
        {
            if (id.endsWith ("rate")) return "Velocidad del LFO.";
            if (id.endsWith ("amt")) return "Cuanto modula el LFO.";
            if (id.endsWith ("delay")) return "El LFO entra despues de este tiempo (vibrato que aparece).";
            if (id.endsWith ("dest")) return "Que mueve el LFO.";
            if (id.endsWith ("div")) return "Division del LFO cuando esta en SYNC.";
            if (id.endsWith ("sync")) return "LFO en tiempo con el DAW.";
            if (id.endsWith ("shape")) return "Forma del LFO.";
        }
        return {};
    }
}

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
    slider.setTooltip (helpFor (id));
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
    slider.setBounds (r.removeFromTop (r.getHeight() - (big ? 20 : 16)));
}

void Knob::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().removeFromBottom (big ? 20 : 16);
    const bool show = slider.isMouseOverOrDragging();
    g.setFont (fonts::sans (big ? 12.5f : 10.5f, true));
    g.setColour (! isEnabled() ? pal::dim.withAlpha (0.5f) : (show ? accentOf (*this).darker (0.3f) : pal::navy));
    g.drawText (show ? slider.getTextFromValue (slider.getValue()) : label, r, juce::Justification::centred, false);
}

Choice::Choice (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& l) : label (l)
{
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (s.getParameter (id)))
        box.addItemList (p->choices, 1);
    box.setTooltip (helpFor (id));
    addAndMakeVisible (box);
    att = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, id, box);
}

void Choice::resized()
{
    auto r = getLocalBounds();
    if (label.isNotEmpty()) r.removeFromTop (14);
    box.setBounds (r.withHeight (juce::jmin (r.getHeight(), 22)));
}

void Choice::paint (juce::Graphics& g)
{
    if (label.isEmpty()) return;
    g.setFont (fonts::sans (10.0f, true));
    g.setColour (pal::navySoft);
    g.drawText (label, getLocalBounds().removeFromTop (14), juce::Justification::centredLeft, false);
}

Led::Led (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& text)
{
    setButtonText (text);
    setTooltip (helpFor (id));
    att = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, id, *this);
}

void Section::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (2.0f);
    aero::glassPanel (g, r, 14.0f, 0.92f);
    if (! hideTitle && title.isNotEmpty())
    {
        g.setFont (fonts::sans (11.5f, true));
        const float tw = (float) juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), title) + 22.0f;
        auto pill = juce::Rectangle<float> (r.getX() + 10.0f, r.getY() + 6.0f, tw, 18.0f);
        aero::glossyPill (g, pill, accentOf (*this));
        g.setColour (juce::Colours::white);
        g.drawText (title, pill, juce::Justification::centred, false);
    }
    if (help.isNotEmpty())
    {
        g.setColour (pal::navySoft);
        g.setFont (fonts::sans (11.5f));
        g.drawFittedText (help, r.withTrimmedTop (30.0f).reduced (14.0f, 9.0f).toNearestInt(), juce::Justification::bottomLeft, 5, 1.0f);
    }
}

void WaveView::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    inset (g, r);
    g.setColour (juce::Colour (0xffcfe6f5));
    g.drawHorizontalLine ((int) r.getCentreY(), r.getX() + 4, r.getRight() - 4);
    dsp::Osc o;
    const int N = 240;
    juce::Path p;
    for (int i = 0; i < N; ++i)
    {
        const float ph = std::fmod (2.0f * (float) i / (float) N, 1.0f);   // two cycles
        o.p = ph; o.q = std::fmod (2.0f * (float) i / (float) N * 2.76f, 1.0f);
        const float y = juce::jlimit (-1.2f, 1.2f, o.tick (wave, 0.004f, shape, 0.0f));
        const float x = r.getX() + 5 + (r.getWidth() - 10) * (float) i / (float) (N - 1);
        const float yy = r.getCentreY() - y * (r.getHeight() * 0.36f);
        if (i == 0) p.startNewSubPath (x, yy); else p.lineTo (x, yy);
    }
    glowStroke (g, p, accentOf (*this), 1.8f);
    if (fmAmt > 0.001f)
    {
        g.setColour (pal::hot);
        g.setFont (fonts::sans (10.0f, true));
        g.drawText ("FM", r.reduced (6, 3), juce::Justification::topRight);
    }
}

void EnvView::set (float a, float d, float s, float r)
{
    if (a == A && d == D && s == S && r == R) return;
    A = a; D = d; S = s; R = r; repaint();
}

void EnvView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (0.5f);
    inset (g, b);
    b = b.reduced (8.0f, 9.0f);
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
    const auto c = accentOf (*this);
    juce::Path fill (p); fill.lineTo (b.getX(), b.getBottom()); fill.closeSubPath();
    juce::ColourGradient grad (c.withAlpha (0.3f), 0, b.getY(), c.withAlpha (0.03f), 0, b.getBottom(), false);
    g.setGradientFill (grad); g.fillPath (fill);
    glowStroke (g, p, c, 1.8f);
}

void FilterView::set (int t, float c, float r)
{
    if (t == type && std::abs (c - cut) < 0.5f && std::abs (r - res) < 0.001f) return;
    type = t; cut = c; res = r; repaint();
}

void FilterView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (0.5f);
    inset (g, b);
    g.setColour (juce::Colour (0xffdcecf7));
    for (float f : { 100.0f, 1000.0f, 10000.0f })
    {
        const float x = b.getX() + b.getWidth() * std::log (f / 20.0f) / std::log (1000.0f);
        g.drawVerticalLine ((int) x, b.getY() + 3, b.getBottom() - 3);
    }
    b = b.reduced (3.0f, 6.0f);
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
    glowStroke (g, p, accentOf (*this), 1.8f);
}

void LevelMeter::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (0.5f);
    inset (g, b, b.getWidth() * 0.5f);
    const int segs = 16;
    const float db = 20.0f * std::log10 (juce::jmax (1.0e-5f, level * 1.414f));
    const float norm = juce::jlimit (0.0f, 1.0f, (db + 48.0f) / 48.0f);
    auto in = b.reduced (4.0f, 6.0f);
    const float sh = in.getHeight() / (float) segs;
    for (int i = 0; i < segs; ++i)
    {
        const bool on = (float) i / (float) segs < norm;
        const auto c = i > 13 ? pal::hot : (i > 10 ? juce::Colour (0xffffc928) : (i > 6 ? pal::leaf : pal::aqua));
        g.setColour (on ? c : juce::Colour (0xffd5e8f4));
        g.fillRoundedRectangle (in.getX(), in.getBottom() - (float) (i + 1) * sh + 1.0f, in.getWidth(), sh - 2.0f, 2.0f);
    }
}

void DiceButton::paintButton (juce::Graphics& g, bool over, bool down)
{
    auto r = getLocalBounds().toFloat().reduced (3.0f);
    const float side = juce::jmin (r.getWidth(), r.getHeight());
    r = r.withSizeKeepingCentre (side, side);
    if (down) r = r.reduced (1.5f);
    juce::Graphics::ScopedSaveState ss (g);
    if (over) g.addTransform (juce::AffineTransform::rotation (-0.12f, r.getCentreX(), r.getCentreY()));
    g.setColour (juce::Colour (0xff0b3556).withAlpha (0.18f));
    g.fillRoundedRectangle (r.translated (1.5f, 2.5f), side * 0.22f);
    aero::drawDice (g, r, face, pips);
}

void RandomButton::paintButton (juce::Graphics& g, bool over, bool down)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    // Y2K candy gradient pill
    const float rad = r.getHeight() * 0.5f;
    juce::ColourGradient grad (juce::Colour (0xffff7ad9), r.getX(), r.getY(), juce::Colour (0xffff8a1e), r.getRight(), r.getBottom(), false);
    grad.addColour (0.5, juce::Colour (0xffff4f6d));
    if (over) grad.multiplyOpacity (1.0f);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (r, rad);
    auto top = r.reduced (3.0f, 2.0f).withHeight (r.getHeight() * 0.48f);
    juce::ColourGradient gl (juce::Colours::white.withAlpha (down ? 0.3f : (over ? 0.85f : 0.7f)), top.getX(), top.getY(),
                             juce::Colours::white.withAlpha (0.05f), top.getX(), top.getBottom(), false);
    g.setGradientFill (gl);
    g.fillRoundedRectangle (top, rad - 2.0f);
    g.setColour (juce::Colour (0xffb0204a).withAlpha (0.6f));
    g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);

    auto d = r.reduced (6.0f).removeFromLeft (r.getHeight() - 12.0f);
    aero::drawDice (g, d, face, juce::Colour (0xffff3d7f));
    g.setFont (fonts::sans (15.0f, true));
    auto tr = r.withTrimmedLeft (d.getRight() - r.getX() + 4.0f);
    g.setColour (juce::Colour (0x40000000));
    g.drawText ("RANDOM TOTAL", tr.translated (0, 1), juce::Justification::centred, false);
    g.setColour (juce::Colours::white);
    g.drawText ("RANDOM TOTAL", tr, juce::Justification::centred, false);
}

CategoryCard::CategoryCard (const juce::String& cat, const juce::String& desc) : category (cat), description (desc)
{
    dice.setTooltip ("Tira el dado: un sonido nuevo de " + cat + " cada vez.");
    dice.onClick = [this] { dice.roll(); if (onDice) onDice(); };
    dice.pips = pal::forCategory (cat).darker (0.4f);
    addAndMakeVisible (dice);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void CategoryCard::resized()
{
    const int s = juce::jmin (52, getHeight() - 20);
    dice.setBounds (getWidth() - s - 14, (getHeight() - s) / 2, s, s);
}

void CategoryCard::mouseUp (const juce::MouseEvent& e)
{
    if (e.mouseWasClicked() && onPick) onPick();
}

void CategoryCard::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (3.0f);
    const auto c = pal::forCategory (category);
    const bool hover = isMouseOver (true);
    const float rad = 16.0f;

    if (active)
    {
        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.fillRoundedRectangle (r.expanded (2.5f), rad + 2.5f);
    }
    g.setColour (pal::navy.withAlpha (0.12f));
    g.fillRoundedRectangle (r.translated (0, 3.0f), rad);
    auto base = hover ? c.brighter (0.12f) : c;
    juce::ColourGradient grad (base.brighter (0.55f), r.getX(), r.getY(), base.darker (0.15f), r.getX(), r.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (r, rad);
    // bubbles inside the card
    aero::bubble (g, { r.getRight() - 90.0f, r.getY() + 22.0f }, 9.0f, 0.5f);
    aero::bubble (g, { r.getRight() - 104.0f, r.getBottom() - 20.0f }, 5.0f, 0.5f);
    auto top = r.reduced (3.0f, 2.0f).withHeight (r.getHeight() * 0.46f);
    juce::ColourGradient gl (juce::Colours::white.withAlpha (0.65f), top.getX(), top.getY(), juce::Colours::white.withAlpha (0.04f), top.getX(), top.getBottom(), false);
    g.setGradientFill (gl);
    g.fillRoundedRectangle (top, rad - 3.0f);
    g.setColour (juce::Colours::white.withAlpha (active ? 1.0f : 0.8f));
    g.drawRoundedRectangle (r.reduced (0.5f), rad, active ? 2.0f : 1.0f);

    auto t = r.reduced (16.0f, 10.0f).withTrimmedRight (60.0f);
    auto shadowText = [&] (const juce::String& s, juce::Rectangle<float> a, float size, bool bold, float alpha)
    {
        g.setFont (fonts::sans (size, bold));
        g.setColour (pal::navy.withAlpha (0.35f * alpha));
        g.drawText (s, a.translated (0, 1.2f), juce::Justification::centredLeft, true);
        g.setColour (juce::Colours::white.withAlpha (alpha));
        g.drawText (s, a, juce::Justification::centredLeft, true);
    };
    shadowText (category, t.removeFromTop (t.getHeight() * 0.45f), 18.0f, true, 1.0f);
    shadowText (description, t.removeFromTop (t.getHeight() * 0.5f), 12.5f, false, 0.95f);
    shadowText (juce::String (count) + " sonidos  -  tira el dado ->", t, 10.5f, true, 0.8f);
}

void VocoderView::update (bool on, int bands)
{
    isOn = on; nb = juce::jlimit (6, Vocoder::maxBands, bands);
    for (int i = 0; i < Vocoder::maxBands; ++i)
    {
        const float v = std::sqrt (juce::jmax (0.0f, voc.bandLevel[i].load (std::memory_order_relaxed))) * 2.2f;
        lv[i] += (v - lv[i]) * (v > lv[i] ? 0.6f : 0.2f);
    }
    gate += (voc.voiceGate.load() - gate) * 0.4f;
    vowel = voc.currentVowel.load();
    repaint();
}

void VocoderView::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (2.0f);
    aero::glassPanel (g, r, 14.0f, 0.92f);
    auto in = r.reduced (16.0f, 14.0f);
    auto letterArea = in.removeFromRight (in.getHeight()).reduced (6.0f);
    in.removeFromRight (16.0f);

    inset (g, in, 10.0f);
    auto bars = in.reduced (10.0f, 10.0f);
    const float bw = bars.getWidth() / (float) nb;
    for (int i = 0; i < nb; ++i)
    {
        const float h = juce::jlimit (0.02f, 1.0f, lv[i]) * bars.getHeight();
        auto b = juce::Rectangle<float> (bars.getX() + bw * (float) i + 2.0f, bars.getBottom() - h, bw - 4.0f, h);
        juce::ColourGradient grad (pal::cyan, 0, bars.getBottom(), pal::leaf, 0, bars.getY(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (b, juce::jmin (5.0f, bw * 0.3f));
        g.setColour (juce::Colours::white.withAlpha (0.5f));
        g.fillRoundedRectangle (b.withWidth (b.getWidth() * 0.35f).translated (2.0f, 0), 2.0f);
    }
    g.setColour (pal::navySoft);
    g.setFont (fonts::sans (10.0f, true));
    g.drawText ("GRAVES", bars.removeFromBottom (14.0f).withTrimmedLeft (2), juce::Justification::bottomLeft);
    g.drawText ("AGUDOS", in.reduced (12.0f, 10.0f), juce::Justification::bottomRight);

    // the vowel bubble
    aero::orb (g, letterArea, pal::forCategory ("VOCODER"), 0.9f);
    static const char* vowels[] = { "A", "E", "I", "O", "U" };
    g.setFont (fonts::sans (letterArea.getHeight() * 0.5f, true));
    g.setColour (juce::Colours::white.withAlpha (0.35f + 0.65f * juce::jlimit (0.0f, 1.0f, gate)));
    g.drawText (vowels[juce::jlimit (0, 4, vowel)], letterArea.translated (0, letterArea.getHeight() * 0.04f), juce::Justification::centred, false);

    if (! isOn)
    {
        g.setColour (juce::Colours::white.withAlpha (0.65f));
        g.fillRoundedRectangle (r, 14.0f);
        g.setColour (pal::navy);
        g.setFont (fonts::sans (20.0f, true));
        g.drawText ("VOCODER APAGADO  -  prendelo arriba o tira el dado", r, juce::Justification::centred, false);
    }
}

//============================================================================== background
void DesgraciaEditor::Content::paint (juce::Graphics& g)
{
    const int W = getWidth(), H = getHeight();
    if (! bgCache.isValid() || bgCache.getWidth() != W)
    {
        bgCache = juce::Image (juce::Image::RGB, W, H, true);
        juce::Graphics b (bgCache);
        // sky
        juce::ColourGradient sky (juce::Colour (0xff2b9df0), 0, 0, juce::Colour (0xffe8f8ff), 0, (float) H * 0.78f, false);
        sky.addColour (0.45, juce::Colour (0xff8fd6ff));
        b.setGradientFill (sky);
        b.fillAll();
        // sun glare + rays
        const juce::Point<float> sun ((float) W * 0.86f, 40.0f);
        for (int i = 0; i < 9; ++i)
        {
            juce::Path ray;
            const float a0 = 1.75f + (float) i * 0.17f, a1 = a0 + 0.06f;
            ray.startNewSubPath (sun);
            ray.lineTo (sun.x + std::cos (a0) * 1400.0f, sun.y + std::sin (a0) * 1400.0f);
            ray.lineTo (sun.x + std::cos (a1) * 1400.0f, sun.y + std::sin (a1) * 1400.0f);
            ray.closeSubPath();
            b.setColour (juce::Colours::white.withAlpha (0.07f));
            b.fillPath (ray);
        }
        juce::ColourGradient glare (juce::Colours::white.withAlpha (0.9f), sun.x, sun.y, juce::Colours::white.withAlpha (0.0f), sun.x + 260.0f, sun.y, true);
        b.setGradientFill (glare);
        b.fillEllipse (sun.x - 260.0f, sun.y - 260.0f, 520.0f, 520.0f);
        // clouds
        auto cloud = [&] (float x, float y, float s)
        {
            b.setColour (juce::Colours::white.withAlpha (0.55f));
            for (auto e : { juce::Rectangle<float> (0, 18, 120, 40), juce::Rectangle<float> (22, 0, 60, 50), juce::Rectangle<float> (62, 8, 50, 40) })
                b.fillEllipse (x + e.getX() * s, y + e.getY() * s, e.getWidth() * s, e.getHeight() * s);
        };
        cloud (60, 110, 1.4f); cloud (430, 70, 1.0f); cloud (700, 150, 1.7f); cloud (250, 300, 1.2f); cloud (1000, 260, 1.1f);
        // water
        auto wave = [&] (float y0, float amp, float len, float phase, juce::Colour top, juce::Colour bot)
        {
            juce::Path p;
            p.startNewSubPath (0, (float) H);
            for (int x = 0; x <= W; x += 8)
                p.lineTo ((float) x, y0 + amp * std::sin ((float) x / len + phase));
            p.lineTo ((float) W, (float) H);
            p.closeSubPath();
            juce::ColourGradient wg (top, 0, y0 - amp, bot, 0, (float) H, false);
            b.setGradientFill (wg);
            b.fillPath (p);
        };
        wave ((float) H * 0.70f, 16.0f, 120.0f, 0.0f, juce::Colour (0x9963c7ff), juce::Colour (0xff0a6fd0));
        wave ((float) H * 0.76f, 12.0f, 90.0f, 1.7f, juce::Colour (0x8819a7ff), juce::Colour (0xff0b5fb0));
        // leaves
        auto leaf = [&] (float x, float y, float len, float ang, juce::Colour c)
        {
            juce::Path p;
            p.startNewSubPath (0, 0);
            p.quadraticTo (len * 0.5f, -len * 0.28f, len, 0);
            p.quadraticTo (len * 0.5f, len * 0.28f, 0, 0);
            const auto t = juce::AffineTransform::rotation (ang).translated (x, y);
            juce::ColourGradient lg (c.brighter (0.5f), x, y, c.darker (0.2f), x + std::cos (ang) * len, y + std::sin (ang) * len, false);
            b.setGradientFill (lg);
            b.fillPath (p, t);
            juce::Path vein; vein.startNewSubPath (0, 0); vein.lineTo (len * 0.9f, 0);
            b.setColour (juce::Colours::white.withAlpha (0.45f));
            b.strokePath (vein, juce::PathStrokeType (1.5f), t);
        };
        leaf (-10, (float) H - 40, 150, -0.6f, pal::leaf);
        leaf (-20, (float) H - 90, 120, -0.2f, juce::Colour (0xff36b02a));
        leaf ((float) W + 10, (float) H - 50, 150, juce::MathConstants<float>::pi + 0.6f, pal::leaf);
        leaf ((float) W + 10, (float) H - 110, 110, juce::MathConstants<float>::pi + 0.15f, juce::Colour (0xff36b02a));
        // static bubbles
        juce::Random r (7);
        for (int i = 0; i < 40; ++i)
            aero::bubble (b, { r.nextFloat() * (float) W, (float) H * 0.35f + r.nextFloat() * (float) H * 0.6f }, 3.0f + r.nextFloat() * 14.0f, 0.35f + r.nextFloat() * 0.4f);

        // glossy header bar
        auto hb = juce::Rectangle<float> (0, 0, (float) W, 60.0f);
        juce::ColourGradient hg (juce::Colour (0xff53c2ff), 0, 0, juce::Colour (0xff0a6fd0), 0, 60.0f, false);
        b.setGradientFill (hg);
        b.fillRect (hb);
        juce::ColourGradient hgl (juce::Colours::white.withAlpha (0.55f), 0, 0, juce::Colours::white.withAlpha (0.05f), 0, 28.0f, false);
        b.setGradientFill (hgl);
        b.fillRect (hb.withHeight (28.0f));
        b.setColour (juce::Colours::white.withAlpha (0.8f));
        b.drawHorizontalLine (59, 0, (float) W);

        // logo
        aero::orb (b, { 14, 10, 40, 40 }, pal::leaf);
        b.setFont (juce::Font (juce::FontOptions (34.0f, juce::Font::bold | juce::Font::italic)));
        b.setColour (juce::Colour (0x55002a50));
        b.drawText ("Desgracia", 62, 7, 230, 40, juce::Justification::centredLeft, false);
        b.setColour (juce::Colours::white);
        b.drawText ("Desgracia", 60, 5, 230, 40, juce::Justification::centredLeft, false);
        b.setFont (fonts::sans (10.5f, true));
        b.setColour (juce::Colours::white.withAlpha (0.85f));
        b.drawText ("sinte digital sucio  -  y2k edition v2.1", 62, 40, 220, 14, juce::Justification::centredLeft, false);
    }
    g.drawImageAt (bgCache, 0, 0);
    g.setColour (accent);
    g.fillRect (0, 60, W, 3);
}

//============================================================================== editor
ui::Knob& DesgraciaEditor::knob (juce::Component& parent, const juce::String& id, const juce::String& label)
{
    auto* k = knobs.add (new ui::Knob (proc.apvts, id, label, [this] (const juce::String& n, const juce::String& v) { showValue (n, v); }));
    parent.addAndMakeVisible (k);
    knobMap[id] = k;
    return *k;
}

ui::Choice& DesgraciaEditor::choice (juce::Component& parent, const juce::String& id, const juce::String& label)
{
    auto* c = choicesList.add (new ui::Choice (proc.apvts, id, label));
    parent.addAndMakeVisible (c);
    choiceMap[id] = c;
    return *c;
}

ui::Led& DesgraciaEditor::led (juce::Component& parent, const juce::String& id, const juce::String& text)
{
    auto* l = leds.add (new ui::Led (proc.apvts, id, text));
    parent.addAndMakeVisible (l);
    l->onClick = [this, l] { if (l->isMouseOver (true)) showValue (l->getButtonText(), l->getToggleState() ? "ON" : "OFF"); };
    ledMap[id] = l;
    return *l;
}

ui::Section& DesgraciaEditor::section (juce::Component& parent, const juce::String& key, const juce::String& title, const juce::String& help)
{
    auto* s = sections.add (new ui::Section (title, help));
    parent.addAndMakeVisible (s);
    s->toBack();
    sectionMap[key] = s;
    return *s;
}

float DesgraciaEditor::param (const juce::String& id) const
{
    auto* r = proc.apvts.getRawParameterValue (id);
    return r != nullptr ? r->load() : 0.0f;
}

DesgraciaEditor::DesgraciaEditor (DesgraciaProcessor& p)
    : AudioProcessorEditor (p), proc (p), vocView (p.vocoder), character (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);
    tooltips.setLookAndFeel (&lnf);
    addAndMakeVisible (content);
    content.setSize (baseW, baseH);

    for (auto& pg : pages) content.addChildComponent (pg);

    // ---------------- TOCAR (play): category cards + preset list
    auto& tocar = pages[0];
    for (auto& ci : catInfo)
    {
        auto* c = cards.add (new ui::CategoryCard (ci.name, ci.desc));
        tocar.addAndMakeVisible (c);
        const juce::String cat (ci.name);
        c->onPick = [this, cat] { browse (cat, true); };
        c->onDice = [this, cat]
        {
            proc.presets.rollDice (cat);
            browseCategory = cat;
            rebuildList();
            character.say ("dado " + cat.toLowerCase() + "!", 1.6f);
        };
    }
    section (tocar, "LIST", "SONIDOS");
    tocar.addAndMakeVisible (presetList);
    presetList.setRowHeight (27);
    presetList.setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    presetList.setOutlineThickness (0);
    tocar.addAndMakeVisible (allBtn);
    allBtn.onClick = [this] { browseCategory = "ALL"; rebuildList(); };
    allBtn.setColour (juce::TextButton::buttonColourId, pal::navySoft);

    // ---------------- SYNTH
    auto& syn = pages[1];
    section (syn, "OSC A", "OSC A");
    section (syn, "OSC B", "OSC B");
    section (syn, "MOD", "MODULACION B>A / PITCH");
    section (syn, "SOURCE", "SUB / RUIDO / UNISON");
    section (syn, "FILTER", "FILTRO");
    section (syn, "VOICE", "VOCES / SALIDA");
    section (syn, "AMP", "VOLUMEN (AMP ENV)");
    section (syn, "FENV", "ENV DE FILTRO (y FM ENV)");
    section (syn, "LFO1", "LFO 1  (+ MOD WHEEL)");
    section (syn, "LFO2", "LFO 2");

    for (auto t : { "A", "B" })
    {
        juce::String o = juce::String ("osc") + t + "_";
        choice (syn, o + "wave");
        knob (syn, o + "oct", "OCT"); knob (syn, o + "semi", "SEMI"); knob (syn, o + "fine", "FINE");
        knob (syn, o + "shape", "SHAPE"); knob (syn, o + "level", "LEVEL");
    }
    syn.addAndMakeVisible (waveA);
    syn.addAndMakeVisible (waveB);

    knob (syn, pid::fmAmt, "FM B>A"); knob (syn, pid::fmEnv, "FM ENV"); knob (syn, pid::ringAmt, "RING");
    knob (syn, pid::penvAmt, "P.ENV"); knob (syn, pid::penvDecay, "P.DECAY"); choice (syn, pid::penvDest, "P.ENV A");

    knob (syn, pid::subLevel, "SUB"); choice (syn, pid::subWave, "SUB ONDA"); choice (syn, pid::subOct, "SUB OCT");
    knob (syn, pid::noiseLevel, "RUIDO"); knob (syn, pid::noiseColor, "COLOR");
    knob (syn, pid::uniVoices, "UNISON"); knob (syn, pid::uniDetune, "DETUNE"); knob (syn, pid::uniSpread, "SPREAD"); knob (syn, pid::drift, "DRIFT");

    choice (syn, pid::fltType);
    knob (syn, pid::fltCutoff, "CUTOFF"); knob (syn, pid::fltReso, "RESO"); knob (syn, pid::fltDrive, "DRIVE");
    knob (syn, pid::fltEnv, "ENV"); knob (syn, pid::fltKey, "KEY"); knob (syn, pid::fltVel, "VEL");
    syn.addAndMakeVisible (filterView);

    choice (syn, pid::voiceMode, "MODO"); knob (syn, pid::glide, "GLIDE"); knob (syn, pid::bendRange, "BEND");
    knob (syn, pid::width, "ANCHO"); knob (syn, pid::master, "MASTER");
    syn.addAndMakeVisible (meter);

    knob (syn, pid::ampA, "A"); knob (syn, pid::ampD, "D"); knob (syn, pid::ampS, "S"); knob (syn, pid::ampR, "R"); knob (syn, pid::ampVel, "VEL");
    knob (syn, pid::fenvA, "A"); knob (syn, pid::fenvD, "D"); knob (syn, pid::fenvS, "S"); knob (syn, pid::fenvR, "R");
    syn.addAndMakeVisible (ampView);
    syn.addAndMakeVisible (fenvView);

    for (int i = 0; i < 2; ++i)
    {
        choice (syn, pid::lfo (i, "shape"));
        knob (syn, pid::lfo (i, "rate"), "RATE"); knob (syn, pid::lfo (i, "amt"), "CANTIDAD"); knob (syn, pid::lfo (i, "delay"), "DELAY");
        choice (syn, pid::lfo (i, "dest"), "MUEVE"); choice (syn, pid::lfo (i, "div"), "SYNC DIV");
        led (syn, pid::lfo (i, "sync"), "SYNC");
    }

    // ---------------- VOCODER
    auto& voc = pages[2];
    section (voc, "VOC", "VOCODER", "Prendelo, elegi que dice y toca: el synth habla solo. Con pocas bandas suena a robot pobre. "
                                    "El dado tira una voz nueva (wa-wa, yoi-yoi o ruidos de robot).").hideTitle = true;
    led (voc, pid::vocOn, "VOCODER ON");
    choice (voc, pid::vocPattern, "QUE DICE");
    choice (voc, pid::vocRate, "CAMBIA CADA");
    knob (voc, pid::vocBands, "BANDAS"); knob (voc, pid::vocShift, "VOZ +/-"); knob (voc, pid::vocQ, "METAL");
    knob (voc, pid::vocSpeed, "REACCION"); knob (voc, pid::vocSib, "SSSS"); knob (voc, pid::vocMix, "MEZCLA");
    voc.addAndMakeVisible (vocView);
    voc.addAndMakeVisible (vocDice);
    vocDice.setTooltip ("Dado de voz: wa-wa, yoi-yoi, ruidos random... sobre el sonido que tengas.");
    vocDice.pips = pal::forCategory ("VOCODER").darker (0.4f);
    vocDice.onClick = [this] { vocDice.roll(); rollVoice(); };

    // ---------------- FX
    auto& fxp = pages[3];
    section (fxp, "DRIVE", "DRIVE", "Saturacion: de calidez a destruccion total. FOLD es el mas raro.").hideTitle = true;
    section (fxp, "CRUSH", "CRUSH", "Bitcrusher: consola vieja, Game Boy, mp3 roto.").hideTitle = true;
    section (fxp, "EQ", "EQ", "Ecualizador de 3 bandas para acomodar el sonido en la mezcla.").hideTitle = true;
    section (fxp, "CHORUS", "CHORUS", "Chorus: ancho y brillo de cassette.").hideTitle = true;
    section (fxp, "DELAY", "DELAY", "Eco. En SYNC sigue el tempo del DAW, PING lo hace saltar de lado a lado.").hideTitle = true;
    section (fxp, "REVERB", "REVERB", "Reverb: de cuarto chico a nube infinita.").hideTitle = true;
    led (fxp, pid::drvOn, "DRIVE"); choice (fxp, pid::drvType, "TIPO"); knob (fxp, pid::drvAmt, "CANTIDAD"); knob (fxp, pid::drvMix, "MEZCLA");
    led (fxp, pid::crsOn, "CRUSH"); knob (fxp, pid::crsBits, "BITS"); knob (fxp, pid::crsRate, "RATE"); knob (fxp, pid::crsMix, "MEZCLA");
    led (fxp, pid::eqOn, "EQ"); knob (fxp, pid::eqLow, "GRAVES"); knob (fxp, pid::eqMid, "MEDIOS"); knob (fxp, pid::eqMidFreq, "FREQ"); knob (fxp, pid::eqHigh, "AGUDOS");
    led (fxp, pid::choOn, "CHORUS"); knob (fxp, pid::choRate, "RATE"); knob (fxp, pid::choDepth, "PROFUND."); knob (fxp, pid::choMix, "MEZCLA");
    led (fxp, pid::dlyOn, "DELAY"); led (fxp, pid::dlySync, "SYNC"); led (fxp, pid::dlyPing, "PING"); choice (fxp, pid::dlyDiv, "DIVISION");
    knob (fxp, pid::dlyTime, "TIEMPO"); knob (fxp, pid::dlyFb, "REPETIC."); knob (fxp, pid::dlyTone, "TONO"); knob (fxp, pid::dlyMix, "MEZCLA");
    led (fxp, pid::revOn, "REVERB"); knob (fxp, pid::revSize, "TAMANO"); knob (fxp, pid::revDamp, "OSCURO"); knob (fxp, pid::revMix, "MEZCLA");

    // ---------------- right column: character + macros
    content.addAndMakeVisible (character);
    section (content, "MACROS", "PERILLAS FACILES",
             "");
    const std::pair<juce::String, const char*> macros[] = { { pid::macBright, "BRILLO" }, { pid::macDirt, "MUGRE" }, { pid::macDetune, "DESAFINE" },
                                                            { pid::macMove, "MOVIMIENTO" }, { pid::macSpace, "ESPACIO" }, { pid::macLength, "LARGO" } };
    for (auto& m : macros) knob (content, m.first, m.second).setBig (true);

    // ---------------- keyboard
    keyboard.setAvailableRange (12, 108);
    keyboard.setLowestVisibleKey (36);
    keyboard.setKeyWidth (25.0f);
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colours::white);
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, pal::navy);
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xff9cc9e6));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, pal::aqua.withAlpha (0.3f));
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, pal::aqua);
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId, pal::navy.withAlpha (0.25f));
    keyboard.setColour (juce::MidiKeyboardComponent::upDownButtonBackgroundColourId, juce::Colour (0xffd9f1ff));
    keyboard.setColour (juce::MidiKeyboardComponent::upDownButtonArrowColourId, pal::navy);
    keyboard.setWantsKeyboardFocus (false);
    content.addAndMakeVisible (keyboard);

    // ---------------- header
    prevBtn.onClick = [this] { proc.presets.step (-1, browseCategory); };
    nextBtn.onClick = [this] { proc.presets.step (1, browseCategory); };
    nameBtn.onClick = [this] { showPresetMenu(); };
    randBtn.onClick = [this]
    {
        proc.presets.randomize();
        randBtn.face = 1 + rng.nextInt (6);
        character.say ("RANDOM TOTAL!!", 1.6f);
    };
    saveBtn.onClick = [this] { savePresetDialog(); };
    initBtn.onClick = [this] { proc.presets.initPatch(); };
    prevBtn.setTooltip ("Preset anterior (dentro de la categoria elegida)");
    nextBtn.setTooltip ("Preset siguiente (dentro de la categoria elegida)");
    nameBtn.setTooltip ("Click: lista de todos los presets por categoria");
    randBtn.setTooltip ("Random en todo sentido: cualquier onda, filtro, FX, vocoder... cualquier cosa.");
    saveBtn.setTooltip ("Guardar el sonido actual como preset tuyo (categoria USER)");
    initBtn.setTooltip ("Volver a un sonido basico para empezar de cero");
    for (auto* b : { &prevBtn, &nextBtn, &nameBtn, &saveBtn, &initBtn })
    {
        b->setColour (juce::TextButton::buttonColourId, juce::Colour (0xff6fcaff));
        content.addAndMakeVisible (b);
    }
    nameBtn.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff3aa8f0));
    content.addAndMakeVisible (randBtn);

    const char* tabNames[] = { "TOCAR", "SYNTH", "VOCODER", "FX" };
    for (int i = 0; i < 4; ++i)
    {
        tabs[i].setButtonText (tabNames[i]);
        tabs[i].setColour (juce::TextButton::buttonColourId, juce::Colour (0xff8fd3ff));
        tabs[i].onClick = [this, i] { showPage (i); };
        content.addAndMakeVisible (tabs[i]);
    }
    tabs[0].setTooltip ("Elegi categoria, tira dados, toca.");
    tabs[1].setTooltip ("El motor completo: osciladores, filtro, envolventes, LFOs.");
    tabs[2].setTooltip ("El synth habla solo: wa-wa-wa, yoi-yoi, ruidos de robot.");
    tabs[3].setTooltip ("Efectos: drive, crush, EQ, chorus, delay, reverb.");

    content.addAndMakeVisible (status);
    status.setFont (fonts::sans (12.5f, true));
    status.setColour (juce::Label::textColourId, pal::navy);
    status.setJustificationType (juce::Justification::centredRight);

    proc.presets.onChanged = [this] { presetDirty = true; };

    layoutContent();
    showPage (0);
    rebuildList();

    setResizable (true, true);
    setResizeLimits (800, 512, 1920, 1230);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) baseW / (double) baseH);
    setSize (1120, 718);
    startTimerHz (30);
}

DesgraciaEditor::~DesgraciaEditor()
{
    proc.presets.onChanged = nullptr;
    stopTimer();
    presetList.setModel (nullptr);
    tooltips.setLookAndFeel (nullptr);
    setLookAndFeel (nullptr);
}

void DesgraciaEditor::paint (juce::Graphics& g) { g.fillAll (pal::sky); }

void DesgraciaEditor::resized()
{
    const float s = (float) getWidth() / (float) baseW;
    content.setTransform (juce::AffineTransform::scale (s));
}

void DesgraciaEditor::showPage (int index)
{
    currentPage = juce::jlimit (0, 3, index);
    for (int i = 0; i < 4; ++i)
    {
        pages[i].setVisible (i == currentPage);
        tabs[i].setToggleState (i == currentPage, juce::dontSendNotification);
    }
    static const char* hints[] = { "elegi una categoria o tira un dado", "pasa el mouse por una perilla para ver que hace",
                                   "el synth habla solo: prendelo o tira el dado", "efectos: todo pasa por aca en orden" };
    status.setText (hints[currentPage], juce::dontSendNotification);
    statusHold = 0;
}

void DesgraciaEditor::layoutContent()
{
    // header
    prevBtn.setBounds (300, 15, 34, 30);
    nameBtn.setBounds (338, 15, 330, 30);
    nextBtn.setBounds (672, 15, 34, 30);
    randBtn.setBounds (722, 10, 210, 40);
    saveBtn.setBounds (946, 15, 96, 30);
    initBtn.setBounds (1048, 15, 70, 30);

    for (int i = 0; i < 4; ++i) tabs[i].setBounds (12 + i * 128, 70, 122, 30);
    status.setBounds (530, 70, 386, 30);

    const juce::Rectangle<int> pageArea (12, 108, 904, 604);
    for (auto& pg : pages) pg.setBounds (pageArea);

    // ---- TOCAR
    {
        const int cw = 278, gap = 8, rows = (cards.size() + 1) / 2, ch = (604 + 6) / rows - 6;
        for (int i = 0; i < cards.size(); ++i)
            cards[i]->setBounds ((i % 2) * (cw + gap), (i / 2) * (ch + 6) - 2, cw, ch);
        auto list = juce::Rectangle<int> (cw * 2 + gap + 10, 0, 904 - (cw * 2 + gap + 10), 604);
        sectionMap["LIST"]->setBounds (list);
        allBtn.setBounds (list.getRight() - 104, list.getY() + 7, 92, 20);
        presetList.setBounds (list.reduced (10, 0).withTrimmedTop (34).withTrimmedBottom (12));
    }

    auto placeK = [&] (const juce::String& id, int x, int y, int w = 54, int h = 62) { if (auto* k = knobMap[id]) k->setBounds (x, y, w, h); };
    auto row = [&] (std::initializer_list<juce::String> ids, int x, int y, int gap = 54, int w = 54, int h = 62)
    { for (auto& id : ids) { placeK (id, x, y, w, h); x += gap; } };
    auto sec = [&] (const juce::String& key, int x, int y, int w, int h) -> juce::Rectangle<int>
    { auto* s = sectionMap[key]; s->setBounds (x, y, w, h); return s->getBounds(); };

    // ---- SYNTH
    {
        const int c1 = 0, c2 = 306, c3 = 612, wA = 300, wC = 292;
        const int r1 = 0, r2 = 176, r3 = 352, r4 = 482;
        for (int i = 0; i < 2; ++i)
        {
            auto b = sec (i == 0 ? "OSC A" : "OSC B", i == 0 ? c1 : c2, r1, wA, 170);
            juce::String o = i == 0 ? "oscA_" : "oscB_";
            choiceMap[o + "wave"]->setBounds (b.getRight() - 112, b.getY() + 5, 100, 20);
            (i == 0 ? waveA : waveB).setBounds (b.getX() + 12, b.getY() + 32, b.getWidth() - 24, 54);
            row ({ o + "oct", o + "semi", o + "fine", o + "shape", o + "level" }, b.getX() + 15, b.getY() + 94);
        }
        {
            auto b = sec ("MOD", c3, r1, wC, 170);
            row ({ pid::fmAmt, pid::fmEnv, pid::ringAmt }, b.getX() + 22, b.getY() + 30, 84);
            row ({ pid::penvAmt, pid::penvDecay }, b.getX() + 22, b.getY() + 98, 84);
            choiceMap[pid::penvDest]->setBounds (b.getX() + 190, b.getY() + 108, 84, 38);
        }
        {
            auto b = sec ("SOURCE", c1, r2, wA, 170);
            placeK (pid::subLevel, b.getX() + 12, b.getY() + 30);
            choiceMap[pid::subWave]->setBounds (b.getX() + 72, b.getY() + 28, 80, 38);
            choiceMap[pid::subOct]->setBounds (b.getX() + 72, b.getY() + 66, 80, 38);
            row ({ pid::noiseLevel, pid::noiseColor }, b.getX() + 168, b.getY() + 30, 62);
            row ({ pid::uniVoices, pid::uniDetune, pid::uniSpread, pid::drift }, b.getX() + 12, b.getY() + 100, 70);
        }
        {
            auto b = sec ("FILTER", c2, r2, wA, 170);
            choiceMap[pid::fltType]->setBounds (b.getRight() - 92, b.getY() + 5, 80, 20);
            row ({ pid::fltCutoff, pid::fltReso, pid::fltDrive }, b.getX() + 8, b.getY() + 30);
            row ({ pid::fltEnv, pid::fltKey, pid::fltVel }, b.getX() + 8, b.getY() + 100);
            filterView.setBounds (b.getX() + 176, b.getY() + 32, 112, 128);
        }
        {
            auto b = sec ("VOICE", c3, r2, wC, 170);
            choiceMap[pid::voiceMode]->setBounds (b.getX() + 14, b.getY() + 36, 92, 38);
            row ({ pid::glide, pid::bendRange }, b.getX() + 116, b.getY() + 30, 62);
            row ({ pid::width, pid::master }, b.getX() + 116, b.getY() + 100, 62);
            meter.setBounds (b.getRight() - 38, b.getY() + 32, 24, 128);
        }
        {
            auto b = sec ("AMP", c1, r3, 446, 124);
            ampView.setBounds (b.getX() + 12, b.getY() + 32, 150, 82);
            row ({ pid::ampA, pid::ampD, pid::ampS, pid::ampR, pid::ampVel }, b.getX() + 170, b.getY() + 44);
        }
        {
            auto b = sec ("FENV", 454, r3, 450, 124);
            fenvView.setBounds (b.getX() + 12, b.getY() + 32, 196, 82);
            row ({ pid::fenvA, pid::fenvD, pid::fenvS, pid::fenvR }, b.getX() + 220, b.getY() + 44);
        }
        for (int i = 0; i < 2; ++i)
        {
            auto b = sec (i == 0 ? "LFO1" : "LFO2", i == 0 ? 0 : 454, r4, i == 0 ? 446 : 450, 122);
            choiceMap[pid::lfo (i, "shape")]->setBounds (b.getRight() - 96, b.getY() + 5, 84, 20);
            row ({ pid::lfo (i, "rate"), pid::lfo (i, "amt"), pid::lfo (i, "delay") }, b.getX() + 10, b.getY() + 40);
            choiceMap[pid::lfo (i, "dest")]->setBounds (b.getX() + 182, b.getY() + 32, 140, 38);
            choiceMap[pid::lfo (i, "div")]->setBounds (b.getX() + 330, b.getY() + 32, 104, 38);
            ledMap[pid::lfo (i, "sync")]->setBounds (b.getX() + 182, b.getY() + 80, 80, 22);
        }
    }

    // ---- VOCODER
    {
        auto b = sec ("VOC", 0, 0, 904, 236);
        ledMap[pid::vocOn]->setBounds (b.getX() + 16, b.getY() + 8, 200, 26);
        choiceMap[pid::vocPattern]->setBounds (b.getX() + 16, b.getY() + 44, 190, 38);
        choiceMap[pid::vocRate]->setBounds (b.getX() + 220, b.getY() + 44, 110, 38);
        vocDice.setBounds (b.getRight() - 150, b.getY() + 40, 120, 120);
        row ({ pid::vocBands, pid::vocShift, pid::vocQ, pid::vocSpeed, pid::vocSib, pid::vocMix }, b.getX() + 16, b.getY() + 96, 84, 70, 80);
        vocView.setBounds (0, 244, 904, 360);
    }

    // ---- FX (2 x 3 big panels)
    {
        const int w = 296, h = 298, kW = 66, kH = 76;
        auto fxsec = [&] (const juce::String& key, int i)
        { return sec (key, (i % 3) * (w + 8), (i / 3) * (h + 8), w, h); };
        auto ledHead = [&] (const juce::String& id, juce::Rectangle<int> b) { ledMap[id]->setBounds (b.getX() + 16, b.getY() + 10, 160, 26); };

        auto b = fxsec ("DRIVE", 0); ledHead (pid::drvOn, b);
        choiceMap[pid::drvType]->setBounds (b.getX() + 20, b.getY() + 70, 100, 38);
        row ({ pid::drvAmt, pid::drvMix }, b.getX() + 134, b.getY() + 52, 74, kW, kH);
        b = fxsec ("CRUSH", 1); ledHead (pid::crsOn, b);
        row ({ pid::crsBits, pid::crsRate, pid::crsMix }, b.getX() + 30, b.getY() + 52, 82, kW, kH);
        b = fxsec ("EQ", 2); ledHead (pid::eqOn, b);
        row ({ pid::eqLow, pid::eqMid, pid::eqMidFreq, pid::eqHigh }, b.getX() + 12, b.getY() + 52, 68, kW, kH);
        b = fxsec ("CHORUS", 3); ledHead (pid::choOn, b);
        row ({ pid::choRate, pid::choDepth, pid::choMix }, b.getX() + 30, b.getY() + 52, 82, kW, kH);
        b = fxsec ("DELAY", 4); ledHead (pid::dlyOn, b);
        row ({ pid::dlyTime, pid::dlyFb, pid::dlyTone, pid::dlyMix }, b.getX() + 12, b.getY() + 52, 68, kW, kH);
        ledMap[pid::dlySync]->setBounds (b.getX() + 18, b.getY() + 150, 70, 24);
        ledMap[pid::dlyPing]->setBounds (b.getX() + 92, b.getY() + 150, 70, 24);
        choiceMap[pid::dlyDiv]->setBounds (b.getX() + 170, b.getY() + 140, 104, 38);
        b = fxsec ("REVERB", 5); ledHead (pid::revOn, b);
        row ({ pid::revSize, pid::revDamp, pid::revMix }, b.getX() + 30, b.getY() + 52, 82, kW, kH);
    }

    // ---- right column
    character.setBounds (928, 108, 340, 362);
    {
        auto b = sec ("MACROS", 928, 478, 340, 234);
        const juce::String ids[] = { pid::macBright, pid::macDirt, pid::macDetune, pid::macMove, pid::macSpace, pid::macLength };
        for (int i = 0; i < 6; ++i)
            placeK (ids[i], b.getX() + 14 + (i % 3) * 106, b.getY() + 34 + (i / 3) * 98, 100, 94);
    }

    keyboard.setBounds (12, 722, 1256, 88);
}

void DesgraciaEditor::rollVoice()
{
    auto set = [this] (const juce::String& id, float v)
    { if (auto* prm = proc.apvts.getParameter (id)) prm->setValueNotifyingHost (prm->convertTo0to1 (v)); };
    static const int pats[] = { 0, 0, 0, 6, 6, 4, 5, 7, 1 };
    static const int rates[] = { 2, 4, 4, 5, 6, 7, 8 };
    set (pid::vocOn, 1.0f);
    set (pid::vocMix, 1.0f);
    set (pid::vocPattern, (float) pats[rng.nextInt (9)]);
    set (pid::vocRate, (float) rates[rng.nextInt (7)]);
    set (pid::vocBands, (float) (8 + rng.nextInt (17)));
    set (pid::vocShift, (rng.nextFloat() - 0.5f) * 12.0f);
    set (pid::vocQ, 0.2f + rng.nextFloat() * 0.7f);
    set (pid::vocSpeed, 0.4f + rng.nextFloat() * 0.5f);
    set (pid::vocSib, rng.nextFloat() * 0.6f);
    character.say (choices::vocPatterns[(int) param (pid::vocPattern)].toLowerCase(), 1.6f);
}

void DesgraciaEditor::showValue (const juce::String& name, const juce::String& value)
{
    status.setText (name + " = " + value, juce::dontSendNotification);
    status.setColour (juce::Label::textColourId, lnf.getAccent().darker (0.4f));
    statusHold = 60;
    character.say (name + " " + value, 1.0f);
}

void DesgraciaEditor::browse (const juce::String& category, bool loadIfOutside)
{
    browseCategory = category;
    rebuildList();
    if (loadIfOutside && proc.presets.getCurrentCategory() != category)
        proc.presets.step (1, category);
}

void DesgraciaEditor::rebuildList()
{
    listRows.clearQuick();
    const auto& e = proc.presets.getEntries();
    for (int i = 0; i < e.size(); ++i)
        if (browseCategory == "ALL" || e[i].category == browseCategory)
            listRows.add (i);
    sectionMap["LIST"]->title = browseCategory == "ALL" ? juce::String ("TODOS LOS SONIDOS (") + juce::String (e.size()) + ")" : browseCategory;
    sectionMap["LIST"]->repaint();
    allBtn.setVisible (browseCategory != "ALL");
    for (auto* c : cards)
    {
        int n = 0;
        for (auto& en : e) if (en.category == c->category) ++n;
        c->setCount (n);
        c->setActive (c->category == browseCategory);
    }
    presetList.updateContent();
    presetList.repaint();
    const int cur = listRows.indexOf (proc.presets.getCurrentIndex());
    if (cur >= 0) presetList.scrollToEnsureRowIsOnscreen (cur);
}

void DesgraciaEditor::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool)
{
    if (! juce::isPositiveAndBelow (row, listRows.size())) return;
    const auto& e = proc.presets.getEntries()[listRows[row]];
    const bool cur = e.name == proc.presets.getCurrentName();
    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (2.0f, 1.5f);
    const auto c = pal::forCategory (e.category);
    if (cur) aero::glossyPill (g, r, c);
    else if (row % 2 == 0) { g.setColour (juce::Colours::white.withAlpha (0.35f)); g.fillRoundedRectangle (r, 8.0f); }
    if (! cur) { g.setColour (c); g.fillEllipse (r.getX() + 8.0f, r.getCentreY() - 4.0f, 8.0f, 8.0f); }
    g.setColour (cur ? juce::Colours::white : pal::navy);
    g.setFont (fonts::sans (13.0f, cur));
    g.drawText (e.name, r.withTrimmedLeft (24.0f).withTrimmedRight (8.0f), juce::Justification::centredLeft, true);
    if (browseCategory == "ALL")
    {
        g.setFont (fonts::sans (9.5f, true));
        g.setColour (cur ? juce::Colours::white.withAlpha (0.85f) : c.darker (0.2f));
        g.drawText (e.category, r.withTrimmedRight (10.0f), juce::Justification::centredRight, true);
    }
}

void DesgraciaEditor::listBoxItemClicked (int row, const juce::MouseEvent&)
{
    if (juce::isPositiveAndBelow (row, listRows.size())) proc.presets.load (listRows[row]);
}

void DesgraciaEditor::refreshPresetDisplay()
{
    const auto name = proc.presets.getCurrentName();
    const auto cat = proc.presets.getCurrentCategory();
    nameBtn.setButtonText (name);
    const auto c = pal::forCategory (cat);
    lnf.setAccent (c);
    content.accent = c;
    character.setTint (c);
    if (name != lastShownPreset)
    {
        const auto blurb = proc.presets.getCurrentBlurb();
        character.say (blurb.isNotEmpty() ? blurb : name, 2.2f);
        character.blink();
        lastShownPreset = name;
    }
    if (proc.presets.getEntries().size() != 0) rebuildList();
    content.repaint();
    repaint();
}

void DesgraciaEditor::timerCallback()
{
    if (presetDirty.exchange (false)) refreshPresetDisplay();

    for (auto* c : cards) c->dice.tick (rng);
    vocDice.tick (rng);

    if (currentPage == 1)
    {
        const int wA = (int) param (pid::oscAWave), wB = (int) param (pid::oscBWave);
        waveA.set (wA, param (pid::oscAShape), param (pid::fmAmt) + param (pid::fmEnv));
        waveB.set (wB, param (pid::oscBShape), 0.0f);
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
            knobMap[pid::lfo (i, "rate")]->setAlpha (sync ? 0.4f : 1.0f);
            choiceMap[pid::lfo (i, "div")]->setEnabled (sync);
            choiceMap[pid::lfo (i, "div")]->setAlpha (sync ? 1.0f : 0.4f);
        }
    }
    else if (currentPage == 2)
    {
        vocView.update (param (pid::vocOn) > 0.5f, (int) param (pid::vocBands));
    }
    else if (currentPage == 3)
    {
        const bool dsync = param (pid::dlySync) > 0.5f;
        knobMap[pid::dlyTime]->setEnabled (! dsync);
        knobMap[pid::dlyTime]->setAlpha (dsync ? 0.4f : 1.0f);
        choiceMap[pid::dlyDiv]->setEnabled (dsync);
        choiceMap[pid::dlyDiv]->setAlpha (dsync ? 1.0f : 0.4f);
    }

    if (statusHold > 0 && --statusHold == 0)
    {
        status.setColour (juce::Label::textColourId, pal::navy);
        showPage (currentPage);
    }
}

void DesgraciaEditor::showPresetMenu()
{
    juce::PopupMenu m;
    const auto& entries = proc.presets.getEntries();
    for (auto& cat : PresetManager::getCategories())
    {
        juce::PopupMenu sub;
        for (int i = 0; i < entries.size(); ++i)
            if (entries[i].category == cat)
                sub.addItem (i + 1, entries[i].name, true, i == proc.presets.getCurrentIndex());
        if (sub.getNumItems() > 0)
            m.addSubMenu (cat, sub, true, nullptr, cat == proc.presets.getCurrentCategory());
    }
    m.addSeparator();
    m.addItem (10000, "volver a leer mis presets");
    m.addItem (10001, "abrir carpeta de mis presets");
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
        if (safe->proc.presets.saveUser (name)) { safe->browseCategory = "USER"; safe->rebuildList(); safe->character.say ("guardado: " + name, 2.0f); }
        else safe->character.say ("no pude guardar :(", 2.0f);
    }), true);
}
