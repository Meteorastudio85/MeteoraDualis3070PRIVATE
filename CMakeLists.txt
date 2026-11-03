cmake_minimum_required(VERSION 3.22)

project(Dualis3070 VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")
endif()

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Dualis3070
    COMPANY_NAME                "Meteora Plugins"
    COMPANY_COPYRIGHT           "Copyright (c) 2026 Meteora Plugins. All rights reserved."
    BUNDLE_ID                   com.meteoraplugins.dualis3070
    VERSION                     1.0.0
    PRODUCT_NAME                "Dualis 3070"
    PLUGIN_MANUFACTURER_CODE    Mtra
    PLUGIN_CODE                 Dl37
    FORMATS                     VST3 AU Standalone
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    VST3_CATEGORIES             Fx Dynamics
    AU_MAIN_TYPE                kAudioUnitType_Effect
)

juce_add_binary_data(DualisData
    SOURCES
        Resources/panel.jpg
        Resources/knob.png
        Resources/power_on.png
        Resources/power_off.png
        Resources/autogain_on.png
        Resources/autogain_off.png
)

target_sources(Dualis3070
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

target_compile_definitions(Dualis3070
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(Dualis3070
    PRIVATE
        DualisData
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
