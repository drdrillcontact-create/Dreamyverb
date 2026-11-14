cmake_minimum_required(VERSION 3.22)
project(DreamyReverb VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(DREAMY_BUILD_TESTS "Build the DSP test executable" OFF)
option(DREAMY_COPY_PLUGIN_AFTER_BUILD "Copy the built plugin into the system plugin folders" ON)

# Prefer an installed JUCE, otherwise fetch a pinned release from GitHub.
find_package(JUCE CONFIG QUIET)
if(NOT JUCE_FOUND)
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.15
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

# AU only exists on Apple platforms; JUCE builds VST3 everywhere.
if(APPLE)
    set(DREAMY_FORMATS VST3 AU)
else()
    set(DREAMY_FORMATS VST3)
endif()

juce_add_plugin(DreamyReverb
    COMPANY_NAME "AuraAudio"
    PLUGIN_NAME "DreamyReverb"
    BUNDLE_ID "com.auraaudio.dreamyreverb"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD ${DREAMY_COPY_PLUGIN_AFTER_BUILD}
    PLUGIN_MANUFACTURER_CODE "AurA"
    PLUGIN_CODE "DrRv"
    FORMATS ${DREAMY_FORMATS}
    VST3_CATEGORIES Fx Reverb
    PRODUCT_NAME "Dreamy Reverb"
)

target_sources(DreamyReverb PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/MetallicKnobLookAndFeel.h
    Source/ReverbEngine.h
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

if(DREAMY_BUILD_TESTS)
    enable_testing()

    add_executable(ReverbTests Tests/ReverbTests.cpp)
    target_include_directories(ReverbTests PRIVATE Source)
    target_compile_definitions(ReverbTests PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
    )
    target_link_libraries(ReverbTests PRIVATE
        juce::juce_audio_basics
        juce::juce_core
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
    )
    add_test(NAME ReverbTests COMMAND ReverbTests)
endif()
