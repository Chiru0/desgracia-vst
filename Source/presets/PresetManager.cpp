#include "PresetManager.h"
#include "Params.h"

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& state) : apvts (state)
{
    rescanUser();
}

juce::File PresetManager::getUserDirectory()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
               .getChildFile ("Desgracia").getChildFile ("Presets");
}

juce::StringArray PresetManager::getCategories()
{
    juce::StringArray c;
    for (auto* s : presetcat::all) c.add (s);
    c.add ("USER");
    return c;
}

void PresetManager::rescanUser()
{
    const auto keepName = getCurrentName();
    entries.clearQuick();
    const auto& f = getFactoryPresets();
    for (int i = 0; i < (int) f.size(); ++i)
        entries.add ({ f[(size_t) i].name, f[(size_t) i].category, f[(size_t) i].blurb, i, {} });

    auto dir = getUserDirectory();
    if (dir.isDirectory())
    {
        auto files = dir.findChildFiles (juce::File::findFiles, true, "*.desgracia");
        files.sort();
        for (auto& file : files)
            entries.add ({ file.getFileNameWithoutExtension(), "USER", "hecho por vos", -1, file });
    }

    for (int i = 0; i < entries.size(); ++i)
        if (entries[i].name == keepName) { current = i; break; }
}

juce::String PresetManager::getCurrentName() const
{
    if (customName.isNotEmpty()) return customName;
    return juce::isPositiveAndBelow (current, entries.size()) ? entries[current].name : juce::String ("Init");
}

juce::String PresetManager::getCurrentCategory() const
{
    if (customName.isNotEmpty()) return "-";
    return juce::isPositiveAndBelow (current, entries.size()) ? entries[current].category : juce::String();
}

juce::String PresetManager::getCurrentBlurb() const
{
    if (customName.isNotEmpty()) return "algo nuevo...";
    return juce::isPositiveAndBelow (current, entries.size()) ? entries[current].blurb : juce::String();
}

void PresetManager::setCurrentNameOnly (const juce::String& name)
{
    for (int i = 0; i < entries.size(); ++i)
        if (entries[i].name == name) { current = i; customName = {}; return; }
    customName = name;
}

void PresetManager::setParam (const juce::String& id, float realValue)
{
    if (auto* p = apvts.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (realValue));
        p->endChangeGesture();
    }
    else
        jassertfalse;   // typo in a preset table
}

void PresetManager::resetAllToDefault()
{
    for (auto* p : apvts.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
        {
            rp->beginChangeGesture();
            rp->setValueNotifyingHost (rp->getDefaultValue());
            rp->endChangeGesture();
        }
}

void PresetManager::load (int index)
{
    if (! juce::isPositiveAndBelow (index, entries.size())) return;
    if (onBeforeLoad) onBeforeLoad();

    const auto& e = entries.getReference (index);
    if (e.factoryIndex >= 0)
    {
        resetAllToDefault();
        for (auto& kv : getFactoryPresets()[(size_t) e.factoryIndex].values)
            setParam (kv.first, kv.second);
    }
    else
    {
        auto xml = juce::XmlDocument::parse (e.file);
        if (xml == nullptr) return;
        auto* stateXml = xml->getChildByName (apvts.state.getType());
        if (stateXml == nullptr) return;
        resetAllToDefault();
        for (auto* child : stateXml->getChildIterator())
        {
            auto id = child->getStringAttribute ("id");
            if (auto* p = apvts.getParameter (id))
            {
                p->beginChangeGesture();
                p->setValueNotifyingHost (p->convertTo0to1 ((float) child->getDoubleAttribute ("value")));
                p->endChangeGesture();
            }
        }
    }
    current = index;
    customName = {};
    apvts.state.setProperty ("presetName", e.name, nullptr);
    if (onChanged) onChanged();
}

void PresetManager::step (int direction, const juce::String& filter)
{
    juce::Array<int> visible;
    for (int i = 0; i < entries.size(); ++i)
        if (filter.isEmpty() || filter == "ALL" || entries[i].category == filter) visible.add (i);
    if (visible.isEmpty()) return;
    int pos = visible.indexOf (current);
    pos = pos < 0 ? (direction > 0 ? 0 : visible.size() - 1)
                  : (pos + direction + visible.size()) % visible.size();
    load (visible[pos]);
}

bool PresetManager::saveUser (const juce::String& rawName)
{
    auto name = juce::File::createLegalFileName (rawName.trim());
    if (name.isEmpty()) return false;
    auto dir = getUserDirectory();
    if (! dir.createDirectory()) return false;

    auto file = dir.getChildFile (name + ".desgracia");
    if (! writePresetFile (file, name)) return false;

    rescanUser();
    for (int i = 0; i < entries.size(); ++i)
        if (entries[i].file == file) current = i;
    customName = {};
    apvts.state.setProperty ("presetName", name, nullptr);
    if (onChanged) onChanged();
    return true;
}

bool PresetManager::writePresetFile (const juce::File& file, const juce::String& name) const
{
    juce::XmlElement root ("DesgraciaPreset");
    root.setAttribute ("name", name);
    root.setAttribute ("version", 1);
    auto state = apvts.copyState();
    state.removeProperty ("presetName", nullptr);
    root.addChildElement (state.createXml().release());
    return root.writeTo (file);
}

void PresetManager::initPatch()
{
    if (onBeforeLoad) onBeforeLoad();
    resetAllToDefault();
    customName = "Init";
    apvts.state.setProperty ("presetName", customName, nullptr);
    if (onChanged) onChanged();
}

// "Smart" randomizer: random, but inside ranges that stay musical and in-character.
void PresetManager::randomize()
{
    juce::Random r;
    auto chance = [&] (float p) { return r.nextFloat() < p; };
    auto range  = [&] (float a, float b) { return a + (b - a) * r.nextFloat(); };
    auto logr   = [&] (float a, float b) { return a * std::pow (b / a, r.nextFloat()); };
    auto pick   = [&] (std::initializer_list<float> l) { return *(l.begin() + r.nextInt ((int) l.size())); };

    if (onBeforeLoad) onBeforeLoad();
    resetAllToDefault();

    setParam (pid::oscAWave, (float) r.nextInt (8));
    setParam (pid::oscALevel, range (0.6f, 0.9f));
    setParam (pid::oscAShape, chance (0.7f) ? range (0.0f, 0.8f) : 0.0f);
    setParam (pid::oscAOct, pick ({ -1, 0, 0, 0, 1 }));

    setParam (pid::oscBWave, (float) r.nextInt (8));
    setParam (pid::oscBLevel, chance (0.3f) ? 0.0f : range (0.15f, 0.5f));
    setParam (pid::oscBShape, range (0.0f, 0.8f));
    setParam (pid::oscBOct, pick ({ -1, 0, 1, 1, 2 }));
    setParam (pid::oscBSemi, pick ({ 0, 0, 0, 7, 5, -5, 3 }));
    setParam (pid::oscBFine, range (-15.0f, 15.0f));

    if (chance (0.3f)) setParam (pid::fmAmt, range (0.05f, 0.35f));
    if (chance (0.3f)) setParam (pid::fmEnv, range (0.1f, 0.5f));
    if (chance (0.15f)) setParam (pid::ringAmt, range (0.2f, 0.6f));
    if (chance (0.3f)) setParam (pid::subLevel, range (0.2f, 0.5f));
    if (chance (0.2f)) setParam (pid::noiseLevel, range (0.02f, 0.15f));
    if (chance (0.3f)) { setParam (pid::uniVoices, (float) (2 + r.nextInt (3))); setParam (pid::uniDetune, range (0.08f, 0.3f)); }
    setParam (pid::drift, range (0.1f, 0.6f));

    setParam (pid::fltType, pick ({ 0, 0, 1, 1, 1, 2, 2, 5 }));
    setParam (pid::fltCutoff, logr (400.0f, 6000.0f));
    setParam (pid::fltReso, range (0.0f, 0.6f));
    setParam (pid::fltEnv, range (-0.15f, 0.55f));
    setParam (pid::fenvD, logr (0.05f, 0.8f));

    if (chance (0.5f))
    { setParam (pid::ampA, 0.0007f); setParam (pid::ampD, logr (0.1f, 0.9f)); setParam (pid::ampS, 0.0f); }
    else
    { setParam (pid::ampA, logr (0.001f, 0.05f)); setParam (pid::ampS, range (0.6f, 0.9f)); }
    setParam (pid::ampR, logr (0.05f, 0.6f));

    if (chance (0.15f)) { setParam (pid::penvAmt, pick ({ 12, -12, 7, 24 })); setParam (pid::penvDecay, logr (0.01f, 0.1f)); }
    if (chance (0.4f)) { setParam (pid::lfo (0, "amt"), range (0.08f, 0.2f)); setParam (pid::lfo (0, "delay"), range (0.2f, 0.5f)); }
    if (chance (0.4f))
    {
        setParam (pid::lfo (1, "dest"), pick ({ 1, 2, 3, 4, 5 }));
        setParam (pid::lfo (1, "amt"), range (0.1f, 0.4f));
        setParam (pid::lfo (1, "shape"), (float) r.nextInt (6));
        setParam (pid::lfo (1, "rate"), logr (0.1f, 8.0f));
    }
    if (chance (0.4f)) { setParam (pid::voiceMode, 2); setParam (pid::glide, range (0.03f, 0.1f)); }

    if (chance (0.5f)) { setParam (pid::drvOn, 1); setParam (pid::drvType, (float) r.nextInt (4)); setParam (pid::drvAmt, range (0.1f, 0.4f)); }
    if (chance (0.4f)) { setParam (pid::crsOn, 1); setParam (pid::crsBits, range (6.0f, 12.0f)); setParam (pid::crsRate, logr (8000.0f, 30000.0f)); setParam (pid::crsMix, range (0.3f, 0.7f)); }
    if (chance (0.2f)) setParam (pid::choOn, 1);
    if (chance (0.35f)) { setParam (pid::dlyOn, 1); setParam (pid::dlyMix, range (0.1f, 0.2f)); }
    if (chance (0.25f)) { setParam (pid::revOn, 1); setParam (pid::revMix, range (0.1f, 0.2f)); }
    setParam (pid::master, -5.0f);

    customName = "Random #" + juce::String (r.nextInt (999));
    apvts.state.setProperty ("presetName", customName, nullptr);
    if (onChanged) onChanged();
}
