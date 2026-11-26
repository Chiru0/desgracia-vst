#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "Params.h"
#include "engine/SynthEngine.h"
#include "fx/FxChain.h"
#include "presets/PresetManager.h"

class DesgraciaProcessor : public juce::AudioProcessor
{
public:
    DesgraciaProcessor();
    ~DesgraciaProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void reset() override { engine.reset(); fx.reset(); }
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "DESGRACIA"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    PresetManager presets;
    juce::MidiKeyboardState keyboardState;
    SynthEngine engine;

    // ---- UI feedback, written by the audio thread, read by the editor
    static constexpr int scopeSize = 1024;
    float scope[scopeSize] {};
    std::atomic<int> scopeWrite { 0 };
    std::atomic<float> outputLevel { 0.0f };

private:
    void readParams (EngineParams& e, FxParams& f) const;
    float get (const juce::String& id) const;

    std::unordered_map<juce::String, std::atomic<float>*> cache;   // filled once in the constructor
    std::atomic<float>* raw (const juce::String& id) const;

    FxChain fx;
    juce::MidiBuffer chunkMidi;
    int maxChunk = 512;
    float levelSmooth = 0.0f;
};
