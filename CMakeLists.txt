cmake_minimum_required(VERSION 3.0)

set(PROJECT_NAME "DamnGoodSound")

set(FORMATS AUv3 VST3 Standalone)

project(${PROJECT_NAME} VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_XCODE_GENERATE_SCHEME OFF)

set_property(GLOBAL PROPERTY USE_FOLDERS YES)

add_subdirectory(Modules/JUCE)

juce_add_plugin("${PROJECT_NAME}"
    COMPANY_NAME "DamnAudioLab"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Dlab
    PLUGIN_CODE Dslv
    FORMATS "${FORMATS}"
    PRODUCT_NAME "${PROJECT_NAME}"
    VST3_CATEGORIES "Fx" "Distortion"
    AU_MAIN_TYPE "kAudioUnitType_Effect"
    MICROPHONE_PERMISSION_ENABLED FALSE
    MICROPHONE_PERMISSION_TEXT "This applicaion requires a permission to use an audio input device of your computer. By Default, Built-In microphone will be used."
    )

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

set(SourecFiles
    Source/Main.h
    Source/Main.cpp)

target_sources(${PROJECT_NAME} PRIVATE ${SourecFiles})

juce_add_binary_data(BinaryData SOURCES Assets/logo.svg Assets/Font/Barlow-Regular.ttf)

set(JUCE_TARGET_HAS_BINARY_DATA)

set(JUCE_DEPENDENCIES
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra)

target_compile_definitions("${PROJECT_NAME}"
    PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries("${PROJECT_NAME}"
    PRIVATE
    BinaryData
    ${JUCE_DEPENDENCIES}
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

juce_generate_juce_header("${PROJECT_NAME}")