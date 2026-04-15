cmake_minimum_required(VERSION 3.22)
project(MiniByter VERSION 1.0.0)

set(JUCE_DIR "/Users/nathanbalducci/JUCE" CACHE PATH "Path to JUCE root")
add_subdirectory(${JUCE_DIR} ${CMAKE_BINARY_DIR}/JUCE)

juce_add_plugin(MiniByter
    COMPANY_NAME                "HSDX"
    COMPANY_WEBSITE             "www.hsdx.am"
    BUNDLE_ID                   "com.hsdx.minibyter"
    PLUGIN_NAME                 "MiniByter"
    PLUGIN_MANUFACTURER_CODE    "Hsdx"
    PLUGIN_CODE                 "MByt"
    FORMATS                     AU VST3 Standalone
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES             "Fx"
    AU_MAIN_TYPE                "kAudioUnitType_Effect"
)

juce_generate_juce_header(MiniByter)

target_sources(MiniByter
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

target_compile_definitions(MiniByter
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1
)

target_link_libraries(MiniByter
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
