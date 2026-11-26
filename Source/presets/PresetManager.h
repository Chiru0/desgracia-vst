#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "FactoryPresets.h"

// Factory + user presets. All methods are message-thread only.
class PresetManager
{
public:
    struct Entry
    {
        juce::String name, category, blurb;
        int factoryIndex = -1;      // >= 0 for factory presets
        juce::File file;            // for user presets
    };

    explicit PresetManager (juce::AudioProcessorValueTreeState& state);

    const juce::Array<Entry>& getEntries() const { return entries; }
    int getCurrentIndex() const { return current; }
    juce::String getCurrentName() const;
    juce::String getCurrentCategory() const;
    juce::String getCurrentBlurb() const;

    void load (int index);
    void step (int direction, const juce::String& categoryFilter);   // prev / next inside filter
    bool saveUser (const juce::String& name);
    bool writePresetFile (const juce::File& file, const juce::String& name) const;   // current state -> .desgracia
    void initPatch();
    void randomize();
    void rescanUser();

    // used by host program changes / state restore
    void setCurrentNameOnly (const juce::String& name);

    static juce::File getUserDirectory();
    static juce::StringArray getCategories();   // includes "USER"

    std::function<void()> onBeforeLoad;   // e.g. engine panic
    std::function<void()> onChanged;      // UI refresh

private:
    void setParam (const juce::String& id, float realValue);
    void resetAllToDefault();

    juce::AudioProcessorValueTreeState& apvts;
    juce::Array<Entry> entries;
    int current = 0;
    juce::String customName;
};
