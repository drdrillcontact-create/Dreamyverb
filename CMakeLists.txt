cmake_minimum_required(VERSION 3.22)
project(DreamyReverb VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(JUCE CONFIG REQUIRED)

juce_add_plugin(DreamyReverb
    COMPANY_NAME "AuraAudio"
    PLUGIN_NAME "DreamyReverb"
    BUNDLE_ID "com.auraaudio.dreamyreverb"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AST_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE "AurA"
    PLUGIN_CODE "DrRv"
    FORMATS VST3 AU
    PRODUCT_NAME "Dreamy Reverb"
)

target_sources(DreamyReverb PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/MetallicKnobLookAndFeel.h
)

target_compile_definitions(DreamyReverb PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(DreamyReverb PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_audio_basics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_graphics
    juce::juce_data_structures
    juce::juce_core
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
