cmake_minimum_required(VERSION 3.24)

project(ACEAudioLeveler VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.15
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_binary_data(LevelerAssets
    SOURCES
        assets/logo.png
        assets/fonts/Isometra-Regular.ttf
        assets/fonts/Inter-Regular.ttf
        assets/fonts/Inter-SemiBold.ttf)

set(LEVELER_SOURCES
    source/PluginProcessor.cpp
    source/PluginEditor.cpp
    source/dsp/LevelerCore.cpp)

foreach(edition FREE PRO)
    if(edition STREQUAL "FREE")
        set(plugin_target LevelerFree)
        set(plugin_name "Leveler Free")
        set(plugin_code LvlF)
        set(edition_flag LEVELER_EDITION_PRO=0)
        set(pro_dsp OFF)
    else()
        set(plugin_target LevelerPro)
        set(plugin_name "Leveler Pro")
        set(plugin_code LvlP)
        set(edition_flag LEVELER_EDITION_PRO=1)
        set(pro_dsp ON)
    endif()

    juce_add_plugin(${plugin_target}
        COMPANY_NAME "ACE Audio"
        BUNDLE_ID com.acea.Leveler${edition}
        PLUGIN_MANUFACTURER_CODE Acea
        PLUGIN_CODE ${plugin_code}
        FORMATS VST3
        PRODUCT_NAME "${plugin_name}"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE
        VST3_CATEGORIES "Fx")

    target_sources(${plugin_target} PRIVATE ${LEVELER_SOURCES})
    if(pro_dsp) # Pro-only DSP modules — absent from the Free binary entirely
        target_sources(${plugin_target} PRIVATE
            source/dsp/Advisor.cpp source/dsp/AutoMode.cpp)
    endif()

    target_compile_definitions(${plugin_target} PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        ${edition_flag})
    if(pro_dsp)
        target_compile_definitions(${plugin_target} PUBLIC LEVELER_BUILD_PRO_DSP=1)
    endif()

    target_link_libraries(${plugin_target} PRIVATE
        LevelerAssets
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags)
endforeach()

# DSP self-test (console): synthetic signals through LevelerCore + Advisor + AutoMode.
# CI runs this before release zips.
add_executable(leveler_selftest
    source/SelfTest.cpp
    source/dsp/LevelerCore.cpp
    source/dsp/Advisor.cpp
    source/dsp/AutoMode.cpp)
target_compile_definitions(leveler_selftest PRIVATE
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 LEVELER_EDITION_PRO=1)
target_link_libraries(leveler_selftest PRIVATE juce::juce_audio_basics juce::juce_dsp)
target_include_directories(leveler_selftest PRIVATE source)
