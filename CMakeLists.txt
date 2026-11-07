cmake_minimum_required(VERSION 3.22)

# macOS deployment target must be set before project()
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")

project(TwinFET VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#-------------------------------------------------------------------------------
# JUCE: a local checkout can be used with  -DJUCE_PATH=/path/to/JUCE
# otherwise a pinned release is downloaded automatically.
set(JUCE_PATH "" CACHE PATH "Optional path to a local JUCE checkout")

if(JUCE_PATH)
    add_subdirectory(${JUCE_PATH} ${CMAKE_BINARY_DIR}/JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.15
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

#-------------------------------------------------------------------------------
juce_add_plugin(TwinFET
    COMPANY_NAME                "FETLab"
    BUNDLE_ID                   com.fetlab.twinfet
    PLUGIN_MANUFACTURER_CODE    Ftlb
    PLUGIN_CODE                 Tf76
    FORMATS                     VST3 AU Standalone
    PRODUCT_NAME                "TwinFET"
    DESCRIPTION                 "FET limiting amplifier: black/blue revisions, fat/clean/analog modes"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    VST3_CATEGORIES             Fx Dynamics
    AU_MAIN_TYPE                kAudioUnitType_Effect)

#-------------------------------------------------------------------------------
# The sources may sit in Source/ dsp/ tests/ (as in the archive) or all together
# in the project root (e.g. after a flat upload to GitHub) - both layouts build.
function(twinfet_locate out name)
    foreach(dir Source dsp tests .)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${dir}/${name}")
            set(${out} "${CMAKE_CURRENT_SOURCE_DIR}/${dir}/${name}" PARENT_SCOPE)
            return()
        endif()
    endforeach()
    message(FATAL_ERROR "TwinFET: cannot find ${name} (looked in Source/, dsp/, tests/ and the project folder)")
endfunction()

set(TWINFET_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}")
foreach(dir Source dsp)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${dir}")
        list(APPEND TWINFET_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/${dir}")
    endif()
endforeach()

twinfet_locate(TWINFET_PROCESSOR_CPP PluginProcessor.cpp)
twinfet_locate(TWINFET_EDITOR_CPP    PluginEditor.cpp)
twinfet_locate(TWINFET_ENGINE_H      TwinFetEngine.h)

target_sources(TwinFET PRIVATE
    ${TWINFET_PROCESSOR_CPP}
    ${TWINFET_EDITOR_CPP})

target_include_directories(TwinFET PRIVATE ${TWINFET_INCLUDE_DIRS})

target_compile_definitions(TwinFET PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

if(MSVC)
    # sources contain UTF-8 text (tooltips)
    target_compile_options(TwinFET PRIVATE /utf-8)
endif()

target_link_libraries(TwinFET
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

#-------------------------------------------------------------------------------
# Offline DSP measurement harness (no JUCE needed):  -DTWINFET_BUILD_TESTS=ON
option(TWINFET_BUILD_TESTS "Build the offline DSP measurement harness" OFF)
if(TWINFET_BUILD_TESTS)
    twinfet_locate(TWINFET_MEASURE_CPP measure.cpp)
    add_executable(twinfet_measure ${TWINFET_MEASURE_CPP})
    target_include_directories(twinfet_measure PRIVATE ${TWINFET_INCLUDE_DIRS})
endif()
