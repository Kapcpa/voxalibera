FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.1
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(Catch2)

juce_add_console_app(custom_daw_tests PRODUCT_NAME "custom-daw-tests")

file(GLOB CUSTOM_DAW_TEST_SOURCES CONFIGURE_DEPENDS *_tests.cpp)
target_sources(custom_daw_tests PRIVATE
    ${CUSTOM_DAW_TEST_SOURCES}
    ${PROJECT_SOURCE_DIR}/source/engine/passthrough.cpp
    ${PROJECT_SOURCE_DIR}/source/platform/alsa_cards.cpp
    ${PROJECT_SOURCE_DIR}/source/platform/pipewire_properties.cpp)
target_include_directories(custom_daw_tests PRIVATE ${PROJECT_SOURCE_DIR}/source)
target_compile_definitions(custom_daw_tests PRIVATE ${CUSTOM_DAW_DEFINITIONS})

target_link_libraries(custom_daw_tests
    PRIVATE
        juce::juce_audio_basics
        PkgConfig::ALSA
        Catch2::Catch2WithMain
    PUBLIC
        juce::juce_recommended_config_flags)

set_source_files_properties(${CUSTOM_DAW_TEST_SOURCES} PROPERTIES COMPILE_OPTIONS "${CUSTOM_DAW_WARNINGS}")

add_test(NAME custom_daw_tests COMMAND custom_daw_tests)
