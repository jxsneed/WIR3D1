cmake_minimum_required(VERSION 3.22)
project(WIR3D VERSION 1.0.0)

include(FetchContent)
FetchContent_Declare(
  JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.6
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(WIR3D
    COMPANY_NAME "Wired Audio"
    BUNDLE_ID "audio.wired.vst"
    PLUGIN_MANUFACTURER_CODE WiRd
    PLUGIN_CODE WiR1
    FORMATS VST3 Standalone
    PRODUCT_NAME "wir3d"
    DESCRIPTION "Cyberpunk vocal autotune, harmonizer ping-pong, beat repeat and matrix filter unit"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(WIR3D)
target_sources(WIR3D PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/PitchDetector.cpp
    Source/ScaleQuantizer.cpp
)
target_compile_features(WIR3D PRIVATE cxx_std_17)
target_compile_definitions(WIR3D PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)
target_link_libraries(WIR3D PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
