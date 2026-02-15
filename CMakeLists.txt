cmake_minimum_required(VERSION 3.22)
project(Tribrato VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

juce_add_binary_data(TribratoBinaryData
    HEADER_NAME BinaryData.h
    NAMESPACE BinaryData
    SOURCES
        tribrato_ui_pngs/tribrato_ui_pngs/knob_highlight.png
        tribrato_ui_pngs/tribrato_ui_pngs/knob_shadow.png
        tribrato_ui_pngs/tribrato_ui_pngs/trigger1_off.png
        tribrato_ui_pngs/tribrato_ui_pngs/trigger1_on.png
        tribrato_ui_pngs/tribrato_ui_pngs/trigger2_off.png
        tribrato_ui_pngs/tribrato_ui_pngs/trigger2_on.png
        tribrato_ui_pngs/tribrato_ui_pngs/trigger3_off.png
        tribrato_ui_pngs/tribrato_ui_pngs/trigger3_on.png
        tribrato_ui_pngs/tribrato_ui_pngs/toggle1_left.png
        tribrato_ui_pngs/tribrato_ui_pngs/toggle1_right.png
        tribrato_ui_pngs/tribrato_ui_pngs/toggle2_left.png
        tribrato_ui_pngs/tribrato_ui_pngs/toggle2_right.png
        tribrato_ui_pngs/tribrato_ui_pngs/toggle3_left.png
        tribrato_ui_pngs/tribrato_ui_pngs/toggle3_right.png
)

juce_add_plugin(Tribrato
    COMPANY_NAME "Tribrato"
    PLUGIN_MANUFACTURER_CODE Trib
    PLUGIN_CODE Trbt
    FORMATS VST3 Standalone
    PRODUCT_NAME "Tribrato"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(Tribrato)

target_sources(Tribrato
    PRIVATE
        Source/VibratoEngine.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

target_compile_definitions(Tribrato
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(Tribrato
    PRIVATE
        TribratoBinaryData
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
