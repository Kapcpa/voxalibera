cmake_minimum_required(VERSION 3.22)
project(voxalibera VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(VOXALIBERA_TESTS "Build the base tests" ON)
option(VOXALIBERA_SANITIZERS "Build with address and undefined behaviour sanitizers" OFF)
option(VOXALIBERA_LTO "Build with link-time optimisation" ON)

if(VOXALIBERA_SANITIZERS)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.15
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(JUCE)

find_package(PkgConfig REQUIRED)
pkg_check_modules(ALSA REQUIRED IMPORTED_TARGET alsa)
pkg_check_modules(SYSTEMD REQUIRED IMPORTED_TARGET libsystemd)

set(VOXALIBERA_WARNINGS
    -Wall -Wextra -Wpedantic -Wshadow=local -Wsign-compare -Wsign-conversion
    -Wunused-parameter -Wunreachable-code -Wuninitialized -Wcast-align -Wswitch-enum
    -Wredundant-decls -Wfloat-equal -Wmissing-field-initializers -Woverloaded-virtual
    -Wreorder -Wzero-as-null-pointer-constant
    -Wno-implicit-fallthrough -Wno-maybe-uninitialized -Wno-ignored-qualifiers)

set(VOXALIBERA_DEFINITIONS
    JUCE_ALSA=1
    JUCE_JACK=1
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_LV2=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)

juce_add_gui_app(voxalibera PRODUCT_NAME "voxalibera")

file(GLOB_RECURSE VOXALIBERA_SOURCES CONFIGURE_DEPENDS source/*.cpp)
target_sources(voxalibera PRIVATE ${VOXALIBERA_SOURCES})
target_include_directories(voxalibera PRIVATE source)

target_compile_definitions(voxalibera PRIVATE
    ${VOXALIBERA_DEFINITIONS}
    JUCE_APPLICATION_NAME_STRING="Voxalibera"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:voxalibera,JUCE_VERSION>")

target_link_libraries(voxalibera
    PRIVATE
        juce::juce_audio_utils
        PkgConfig::ALSA
        PkgConfig::SYSTEMD
    PUBLIC
        juce::juce_recommended_config_flags)

if(VOXALIBERA_LTO)
    target_link_libraries(voxalibera PUBLIC juce::juce_recommended_lto_flags)
endif()

set_source_files_properties(${VOXALIBERA_SOURCES} PROPERTIES COMPILE_OPTIONS "${VOXALIBERA_WARNINGS}")

if(VOXALIBERA_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
