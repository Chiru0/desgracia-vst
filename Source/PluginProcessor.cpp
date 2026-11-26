#include "PluginProcessor.h"
#include "ui/PluginEditor.h"

DesgraciaProcessor::DesgraciaProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "DESGRACIA", createParameterLayout()),
      presets (apvts)
{
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            cache[rp->getParameterID()] = apvts.getRawParameterValue (rp->getParameterID());

    presets.onBeforeLoad = [this] { engine.requestPanic(); };
    presets.load (0);   // start on the signature sound
}

std::atomic<float>* DesgraciaProcessor::raw (const juce::String& id) const
{
    auto it = cache.find (id);
    jassert (it != cache.end());
    return it != cache.end() ? it->second : nullptr;
}

float DesgraciaProcessor::get (const juce::String& id) const
{
    auto* r = raw (id);
    return r != nullptr ? r->load (std::memory_order_relaxed) : 0.0f;
}

bool DesgraciaProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet().isDisabled();
}

void DesgraciaProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    maxChunk = juce::jmax (32, juce::jmin (samplesPerBlock, 2048));
    engine.prepare (sampleRate);
    fx.prepare (sampleRate, maxChunk);
    chunkMidi.ensureSize (8192);
    vocoder.prepare (sampleRate);
    keyboardState.reset();
}

void DesgraciaProcessor::readParams (EngineParams& e, FxParams& f, VocoderParams& v) const
{
    auto i = [this] (const juce::String& id) { return (int) std::lround (get (id)); };
    auto b = [this] (const juce::String& id) { return get (id) > 0.5f; };

    e.a = { i (pid::oscAWave), i (pid::oscAOct), i (pid::oscASemi), get (pid::oscAFine), get (pid::oscALevel), get (pid::oscAShape) };
    e.b = { i (pid::oscBWave), i (pid::oscBOct), i (pid::oscBSemi), get (pid::oscBFine), get (pid::oscBLevel), get (pid::oscBShape) };
    e.fm = get (pid::fmAmt); e.fmEnv = get (pid::fmEnv); e.ring = get (pid::ringAmt);
    e.subWave = i (pid::subWave); e.subOct = i (pid::subOct); e.subLevel = get (pid::subLevel);
    e.noiseLevel = get (pid::noiseLevel); e.noiseColor = get (pid::noiseColor);
    e.uniVoices = i (pid::uniVoices); e.uniDetune = get (pid::uniDetune); e.uniSpread = get (pid::uniSpread); e.drift = get (pid::drift);
    e.fltType = i (pid::fltType); e.cutoff = get (pid::fltCutoff); e.reso = get (pid::fltReso); e.fltDrive = get (pid::fltDrive);
    e.fltEnv = get (pid::fltEnv); e.fltKey = get (pid::fltKey); e.fltVel = get (pid::fltVel);
    e.ampA = get (pid::ampA); e.ampD = get (pid::ampD); e.ampS = get (pid::ampS); e.ampR = get (pid::ampR); e.ampVel = get (pid::ampVel);
    e.fA = get (pid::fenvA); e.fD = get (pid::fenvD); e.fS = get (pid::fenvS); e.fR = get (pid::fenvR);
    e.penvAmt = get (pid::penvAmt); e.penvDecay = get (pid::penvDecay); e.penvDest = i (pid::penvDest);
    for (int k = 0; k < 2; ++k)
    {
        auto& l = e.lfo[k];
        l.shape = i (pid::lfo (k, "shape")); l.rate = get (pid::lfo (k, "rate")); l.sync = b (pid::lfo (k, "sync"));
        l.div = i (pid::lfo (k, "div")); l.dest = i (pid::lfo (k, "dest")); l.amt = get (pid::lfo (k, "amt")); l.delay = get (pid::lfo (k, "delay"));
    }
    e.voiceMode = i (pid::voiceMode); e.glide = get (pid::glide); e.bendRange = i (pid::bendRange);

    f.drvOn = b (pid::drvOn); f.drvType = i (pid::drvType); f.drvAmt = get (pid::drvAmt); f.drvMix = get (pid::drvMix);
    f.crsOn = b (pid::crsOn); f.crsBits = get (pid::crsBits); f.crsRate = get (pid::crsRate); f.crsMix = get (pid::crsMix);
    f.choOn = b (pid::choOn); f.choRate = get (pid::choRate); f.choDepth = get (pid::choDepth); f.choMix = get (pid::choMix);
    f.dlyOn = b (pid::dlyOn); f.dlySync = b (pid::dlySync); f.dlyDiv = i (pid::dlyDiv); f.dlyTime = get (pid::dlyTime);
    f.dlyFb = get (pid::dlyFb); f.dlyTone = get (pid::dlyTone); f.dlyPing = b (pid::dlyPing); f.dlyMix = get (pid::dlyMix);
    f.revOn = b (pid::revOn); f.revSize = get (pid::revSize); f.revDamp = get (pid::revDamp); f.revMix = get (pid::revMix);
    f.eqOn = b (pid::eqOn); f.eqLow = get (pid::eqLow); f.eqMid = get (pid::eqMid); f.eqMidFreq = get (pid::eqMidFreq); f.eqHigh = get (pid::eqHigh);
    f.width = get (pid::width); f.masterDb = get (pid::master);

    v.on = b (pid::vocOn); v.pattern = i (pid::vocPattern); v.rateDiv = i (pid::vocRate);
    v.bands = i (pid::vocBands); v.shift = get (pid::vocShift); v.q = get (pid::vocQ); v.speed = get (pid::vocSpeed);
    v.sib = get (pid::vocSib); v.mix = get (pid::vocMix);

    // ---- macros: offsets on top of whatever the patch is
    const float bright = get (pid::macBright), dirt = get (pid::macDirt), detune = get (pid::macDetune);
    const float move = get (pid::macMove), space = get (pid::macSpace), length = get (pid::macLength);
    e.cutoff = juce::jlimit (20.0f, 20000.0f, e.cutoff * std::exp2 (bright * 3.0f));
    if (bright > 0.0f && f.eqOn) f.eqHigh = juce::jmin (12.0f, f.eqHigh + bright * 4.0f);
    if (dirt > 0.0f)
    {
        e.fltDrive = juce::jmin (1.0f, e.fltDrive + dirt * 0.5f);
        if (f.drvOn) f.drvAmt = juce::jmin (1.0f, f.drvAmt + dirt * 0.45f);
        else { f.drvOn = dirt > 0.02f; f.drvAmt = dirt * 0.55f; f.drvMix = 1.0f; f.drvType = 0; }
        if (f.crsOn) f.crsBits = juce::jmax (3.0f, f.crsBits - dirt * 4.0f);
    }
    e.drift = juce::jmin (1.0f, e.drift + detune * 0.6f);
    e.uniDetune = juce::jmin (1.0f, e.uniDetune + detune * 0.3f);
    if (move > 0.0f)
    {
        bool any = false;
        for (auto& l : e.lfo) if (l.amt > 0.0f) { l.amt = juce::jmin (1.0f, l.amt + move * 0.3f); any = true; }
        if (! any) { e.lfo[1].dest = 1; e.lfo[1].amt = move * 0.45f; e.lfo[1].shape = 5; e.lfo[1].sync = false; e.lfo[1].rate = 0.4f + move * 2.0f; }
    }
    if (space > 0.0f)
    {
        if (! f.revOn) { f.revOn = space > 0.02f; f.revMix = 0.0f; f.revSize = 0.7f; }
        f.revMix = juce::jmin (1.0f, f.revMix + space * 0.4f);
        if (! f.dlyOn) { f.dlyOn = space > 0.02f; f.dlyMix = 0.0f; f.dlyFb = 0.35f; f.dlySync = true; f.dlyDiv = 6; }
        f.dlyMix = juce::jmin (1.0f, f.dlyMix + space * 0.22f);
    }
    const float lm = std::exp2 (length * 2.0f);
    e.ampD *= lm; e.ampR *= lm; e.fD *= lm; e.fR *= lm;
}

void DesgraciaProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int total = buffer.getNumSamples();

    buffer.clear();

    keyboardState.processNextMidiBuffer (midi, 0, total, true);

    EngineParams ep; FxParams fp; VocoderParams vp;
    readParams (ep, fp, vp);
    engine.setParams (ep);

    double bpm = 120.0, ppq = 0.0; bool playing = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto q = pos->getPpqPosition()) ppq = *q;
            playing = pos->getIsPlaying();
        }

    const double sr = getSampleRate() > 0 ? getSampleRate() : 44100.0;
    for (int start = 0; start < total; start += maxChunk)
    {
        const int n = juce::jmin (maxChunk, total - start);
        chunkMidi.clear();
        chunkMidi.addEvents (midi, start, n, -start);

        engine.setTransport (bpm, playing, ppq + (double) start / sr * bpm / 60.0);
        juce::AudioBuffer<float> chunk (buffer.getArrayOfWritePointers(), 2, start, n);
        const bool wasSilent = engine.activeVoices.load() == 0;
        const int before = engine.noteOnCounter.load();
        engine.render (chunk, chunkMidi);
        if (wasSilent && engine.noteOnCounter.load() != before && ! playing) vocoder.retrigger();
        vocoder.process (chunk.getWritePointer (0), chunk.getWritePointer (1), n, vp, bpm, playing, ppq + (double) start / sr * bpm / 60.0);
        fx.process (chunk, fp, bpm);
    }

    // scope + level for the UI (lock-free; tearing is harmless here)
    const float* L = buffer.getReadPointer (0);
    const float* R = buffer.getReadPointer (1);
    int w = scopeWrite.load (std::memory_order_relaxed);
    float sum = 0.0f;
    for (int s = 0; s < total; ++s)
    {
        const float m = 0.5f * (L[s] + R[s]);
        sum += m * m;
        scope[w] = m;
        w = (w + 1) & (scopeSize - 1);
    }
    scopeWrite.store (w, std::memory_order_release);
    const float rms = total > 0 ? std::sqrt (sum / (float) total) : 0.0f;
    levelSmooth = juce::jmax (rms, levelSmooth * 0.85f);
    outputLevel.store (levelSmooth, std::memory_order_relaxed);
}

int DesgraciaProcessor::getNumPrograms() { return (int) getFactoryPresets().size(); }

int DesgraciaProcessor::getCurrentProgram()
{
    const int i = presets.getCurrentIndex();
    return juce::jlimit (0, getNumPrograms() - 1, i);
}

void DesgraciaProcessor::setCurrentProgram (int index)
{
    if (juce::isPositiveAndBelow (index, getNumPrograms()) && index != presets.getCurrentIndex())
        presets.load (index);
}

const juce::String DesgraciaProcessor::getProgramName (int index)
{
    const auto& f = getFactoryPresets();
    return juce::isPositiveAndBelow (index, (int) f.size()) ? juce::String (f[(size_t) index].name) : juce::String();
}

void DesgraciaProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("presetName", presets.getCurrentName(), nullptr);
    state.setProperty ("version", 1, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void DesgraciaProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType())) return;

    auto tree = juce::ValueTree::fromXml (*xml);
    // set every parameter explicitly (missing ones fall back to defaults)
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
        {
            auto child = tree.getChildWithProperty ("id", rp->getParameterID());
            const float norm = child.isValid() ? rp->convertTo0to1 ((float) child.getProperty ("value"))
                                               : rp->getDefaultValue();
            rp->setValueNotifyingHost (norm);
        }
    const auto name = tree.getProperty ("presetName").toString();
    apvts.state.setProperty ("presetName", name, nullptr);
    presets.setCurrentNameOnly (name);
    if (presets.onChanged) presets.onChanged();
}

juce::AudioProcessorEditor* DesgraciaProcessor::createEditor() { return new DesgraciaEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new DesgraciaProcessor(); }
