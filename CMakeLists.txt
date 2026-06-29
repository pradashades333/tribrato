cmake_minimum_required(VERSION 3.22)
project(Tribrato VERSION 1.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

set(NEW_GUI_DIR "New Gui/Tribrato new gui/Tribrato new gui")

juce_add_binary_data(TribratoBinaryData
    HEADER_NAME BinaryData.h
    NAMESPACE BinaryData
    SOURCES
        "${NEW_GUI_DIR}/Full_bg2.png"
        "${NEW_GUI_DIR}/divider_line.png"
        "${NEW_GUI_DIR}/enable_hover.png"
        "${NEW_GUI_DIR}/enable_off.png"
        "${NEW_GUI_DIR}/enable_on.png"
        "${NEW_GUI_DIR}/knob1.png"
        "${NEW_GUI_DIR}/knob1_hover.png"
        "${NEW_GUI_DIR}/knob_shadow.png"
        "${NEW_GUI_DIR}/latch_on.png"
        "${NEW_GUI_DIR}/momentary_on.png"
        "${NEW_GUI_DIR}/redo_hover.png"
        "${NEW_GUI_DIR}/redo_off.png"
        "${NEW_GUI_DIR}/redo_on.png"
        "${NEW_GUI_DIR}/sett_hover.png"
        "${NEW_GUI_DIR}/sett_off.png"
        "${NEW_GUI_DIR}/tip_hover.png"
        "${NEW_GUI_DIR}/tip_off.png"
        "${NEW_GUI_DIR}/tip_on.png"
        "${NEW_GUI_DIR}/trigg_off.png"
        "${NEW_GUI_DIR}/trigg_on.png"
        "${NEW_GUI_DIR}/undo_hover.png"
        "${NEW_GUI_DIR}/undo_off.png"
        "${NEW_GUI_DIR}/undo_on.png"
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
