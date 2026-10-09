cmake_minimum_required(VERSION 3.22)
project(custom_daw VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(CUSTOM_DAW_TESTS "Build the base tests" ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.15
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(JUCE)

find_package(PkgConfig REQUIRED)
pkg_check_modules(ALSA REQUIRED IMPORTED_TARGET alsa)
pkg_check_modules(SYSTEMD REQUIRED IMPORTED_TARGET libsystemd)

set(CUSTOM_DAW_WARNINGS
    -Wall -Wextra -Wpedantic -Wshadow=local -Wsign-compare -Wsign-conversion
    -Wunused-parameter -Wunreachable-code -Wuninitialized -Wcast-align -Wswitch-enum
    -Wredundant-decls -Wfloat-equal -Wmissing-field-initializers -Woverloaded-virtual
    -Wreorder -Wzero-as-null-pointer-constant
    -Wno-implicit-fallthrough -Wno-maybe-uninitialized -Wno-ignored-qualifiers)

set(CUSTOM_DAW_DEFINITIONS
    JUCE_ALSA=1
    JUCE_JACK=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)

juce_add_gui_app(custom_daw PRODUCT_NAME "custom-daw")

file(GLOB_RECURSE CUSTOM_DAW_SOURCES CONFIGURE_DEPENDS source/*.cpp)
target_sources(custom_daw PRIVATE ${CUSTOM_DAW_SOURCES})
target_include_directories(custom_daw PRIVATE source)

target_compile_definitions(custom_daw PRIVATE
    ${CUSTOM_DAW_DEFINITIONS}
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:custom_daw,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:custom_daw,JUCE_VERSION>")

target_link_libraries(custom_daw
    PRIVATE
        juce::juce_audio_utils
        PkgConfig::ALSA
        PkgConfig::SYSTEMD
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags)

set_source_files_properties(${CUSTOM_DAW_SOURCES} PROPERTIES COMPILE_OPTIONS "${CUSTOM_DAW_WARNINGS}")

if(CUSTOM_DAW_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
