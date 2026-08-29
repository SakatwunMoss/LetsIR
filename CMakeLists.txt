cmake_minimum_required(VERSION 3.22)

project(LetsIR VERSION 0.0.1)

add_subdirectory(JUCE)

juce_add_plugin(LetsIR
    COMPANY_NAME "LetsIRDev"
    PLUGIN_MANUFACTURER_CODE LtIr
    PLUGIN_CODE Lsir
    FORMATS Standalone VST3
    PRODUCT_NAME "LetsIR"
    NEEDS_WEB_BROWSER FALSE
    NEEDS_CURL FALSE
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Microphone access is required for recording and input level metering."
    NSBluetoothAlwaysUsageDescription "This app uses Bluetooth to detect audio devices.")

target_sources(LetsIR
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/SweepGenerator.cpp
        Source/Deconvolver.cpp
        Source/IRPostProcessor.cpp
        Source/IRWavExporter.cpp
        Source/IRWaveformComponent.cpp
        Source/InputLevelMeterComponent.cpp)

target_compile_definitions(LetsIR
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(LetsIR
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
