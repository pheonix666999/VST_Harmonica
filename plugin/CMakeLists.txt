cmake_minimum_required(VERSION 3.22)

project(TurboTuning VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ── Static runtime on Windows (no VC++ Redist needed) ───────
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# ── JUCE Dependency ────────────────────────────────────────
set(JUCE_SOURCE_DIR "${JUCE_SOURCE_DIR}" CACHE PATH "Path to a local JUCE source checkout")

if(JUCE_SOURCE_DIR AND EXISTS "${JUCE_SOURCE_DIR}/CMakeLists.txt")
    message(STATUS "Using local JUCE from ${JUCE_SOURCE_DIR}")
    add_subdirectory("${JUCE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/juce")
else()
    include(FetchContent)
    FetchContent_Declare(
        juce
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.1
    )
    FetchContent_MakeAvailable(juce)
endif()

# ── macOS ───────────────────────────────────────────────────
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version")
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "macOS architectures")
    add_compile_options(-Wno-deprecated-declarations)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=unavailable-declarations")
endif()

# ── Plugin Target ───────────────────────────────────────────
juce_add_plugin(TurboTuning
    COMPANY_NAME            "TurboHarp"
    BUNDLE_ID               com.turboharp.turbotuning
    IS_SYNTH                TRUE
    NEEDS_MIDI_INPUT        TRUE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Thpr
    PLUGIN_CODE              TbTn
    FORMATS                 VST3 AU Standalone
    PRODUCT_NAME            "Turbo Harp"
    AU_MAIN_TYPE            kAudioUnitType_MusicDevice
)

juce_generate_juce_header(TurboTuning)

# ── Sources ─────────────────────────────────────────────────
target_sources(TurboTuning PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h

    # DSP
    Source/DSP/HarmonicaEngine.h
    Source/DSP/HarmonicaEngine.cpp
    Source/DSP/BrassEngine.h
    Source/DSP/BrassEngine.cpp
    Source/DSP/StringEngine.h
    Source/DSP/StringEngine.cpp
    Source/DSP/WoodwindEngine.h
    Source/DSP/WoodwindEngine.cpp
    Source/DSP/EffectsChain.h
    Source/DSP/EffectsChain.cpp
    Source/DSP/SafeMode.h
    Source/DSP/SafeMode.cpp
    Source/DSP/AutoTuner.h
    Source/DSP/AutoTuner.cpp

    # UI
    Source/UI/TurboLookAndFeel.h
    Source/UI/TurboLookAndFeel.cpp
    Source/UI/DawEditor.h
    Source/UI/DawEditor.cpp
    Source/UI/HarmonicaView.h
    Source/UI/HarmonicaView.cpp
    Source/UI/CircleOfFifths.h
    Source/UI/CircleOfFifths.cpp
    Source/UI/InstrumentPanel.h
    Source/UI/InstrumentPanel.cpp
    Source/UI/EffectsPanel.h
    Source/UI/EffectsPanel.cpp

    # Util
    Source/Util/MusicTheory.h
)

# ── Compile Definitions ────────────────────────────────────
target_compile_definitions(TurboTuning PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

# ── Link Libraries ─────────────────────────────────────────
target_link_libraries(TurboTuning PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_plugin_client
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
)

if(WIN32)
    target_compile_definitions(TurboTuning PRIVATE NOMINMAX)
endif()

if(TARGET TurboTuning_Standalone)
    add_custom_command(TARGET TurboTuning_Standalone POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_LIST_DIR}/../demo/logo.png"
            "$<TARGET_FILE_DIR:TurboTuning_Standalone>/logo.png")
endif()
