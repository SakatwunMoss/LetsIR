cmake_minimum_required(VERSION 3.22)

project(LetsIR VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

juce_add_gui_app(LetsIR
    COMPANY_NAME "LetsIRDev"
    PRODUCT_NAME "LetsIR"
    NEEDS_WEB_BROWSER FALSE
    NEEDS_CURL FALSE)

target_sources(LetsIR
    PRIVATE
        Source/Main.cpp
        Source/MainComponent.cpp
        Source/IRWaveformComponent.cpp
        Source/Acoustics/MaterialLibrary.cpp
        Source/Acoustics/RoomModel.cpp
        Source/Acoustics/ChannelEncoder.cpp
        Source/Acoustics/AcousticSimulator.cpp
        Source/Acoustics/IRExporter.cpp
        Source/Acoustics/RoomLayoutView.cpp)

target_include_directories(LetsIR
    PRIVATE
        Source)

target_compile_definitions(LetsIR
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:LetsIR,JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:LetsIR,JUCE_VERSION>")

target_link_libraries(LetsIR
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_formats
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
