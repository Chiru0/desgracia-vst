cmake_minimum_required(VERSION 3.22)
project(Desgracia VERSION 2.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(DESGRACIA_BUILD_TESTS "Build the offline render / regression test tool" OFF)

# JUCE: uses ./JUCE if present, otherwise downloads JUCE 8.0.4
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt)
    add_subdirectory(JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_binary_data(DesgraciaAssets SOURCES assets/desgracia.png)

set(DESGRACIA_SOURCES
    Source/Params.cpp
    Source/PluginProcessor.cpp
    Source/engine/SynthEngine.cpp
    Source/fx/FxChain.cpp
    Source/fx/Vocoder.cpp
    Source/presets/FactoryPresets.cpp
    Source/presets/PresetManager.cpp
    Source/ui/DesgraciaLookAndFeel.cpp
    Source/ui/DesgraciaCharacter.cpp
    Source/ui/PluginEditor.cpp)

juce_add_plugin(Desgracia
    COMPANY_NAME "Ciro"
    PLUGIN_MANUFACTURER_CODE Ciro
    PLUGIN_CODE Dsgr
    PRODUCT_NAME "Desgracia"
    PLUGIN_NAME "DESGRACIA"
    DESCRIPTION "Raw underground digital synth"
    FORMATS VST3 Standalone
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    VST3_CATEGORIES Instrument Synth
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(Desgracia PRIVATE ${DESGRACIA_SOURCES})
target_include_directories(Desgracia PRIVATE Source)

set(DESGRACIA_DEFS
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_compile_definitions(Desgracia PUBLIC ${DESGRACIA_DEFS})

target_link_libraries(Desgracia
    PRIVATE DesgraciaAssets juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

if(DESGRACIA_BUILD_TESTS)
    # Console tool: instantiates the real processor, loads every preset,
    # plays MIDI, renders WAVs, checks NaN / silence / stuck notes / state.
    juce_add_console_app(DesgraciaTest PRODUCT_NAME "DesgraciaTest")
    target_sources(DesgraciaTest PRIVATE tests/RenderTest.cpp ${DESGRACIA_SOURCES})
    target_include_directories(DesgraciaTest PRIVATE Source)
    target_compile_definitions(DesgraciaTest PRIVATE ${DESGRACIA_DEFS}
        JucePlugin_Name="DESGRACIA" JucePlugin_IsSynth=1 JucePlugin_WantsMidiInput=1
        JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0 JucePlugin_VersionString="2.0.0"
        JUCE_STANDALONE_APPLICATION=1 JUCE_MODAL_LOOPS_PERMITTED=1)
    target_link_libraries(DesgraciaTest PRIVATE DesgraciaAssets juce::juce_audio_utils juce::juce_dsp
        juce::juce_recommended_config_flags)
endif()
