FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.1
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(Catch2)

juce_add_console_app(voxalibera_tests PRODUCT_NAME "voxalibera-tests")

file(GLOB VOXALIBERA_TEST_SOURCES CONFIGURE_DEPENDS *_tests.cpp)
target_sources(voxalibera_tests PRIVATE
    ${VOXALIBERA_TEST_SOURCES}
    ${PROJECT_SOURCE_DIR}/source/core/project/edit_history.cpp
    ${PROJECT_SOURCE_DIR}/source/core/project/model.cpp
    ${PROJECT_SOURCE_DIR}/source/core/project/project_file.cpp
    ${PROJECT_SOURCE_DIR}/source/engine/audio_file.cpp
    ${PROJECT_SOURCE_DIR}/source/engine/levels.cpp
    ${PROJECT_SOURCE_DIR}/source/engine/mix_renderer.cpp
    ${PROJECT_SOURCE_DIR}/source/engine/musical_time.cpp
    ${PROJECT_SOURCE_DIR}/source/engine/passthrough.cpp
    ${PROJECT_SOURCE_DIR}/source/engine/resampler.cpp
    ${PROJECT_SOURCE_DIR}/source/engine/transport.cpp
    ${PROJECT_SOURCE_DIR}/source/platform/alsa_cards.cpp
    ${PROJECT_SOURCE_DIR}/source/platform/pipewire_properties.cpp)
target_include_directories(voxalibera_tests PRIVATE ${PROJECT_SOURCE_DIR}/source)
target_compile_definitions(voxalibera_tests PRIVATE ${VOXALIBERA_DEFINITIONS})

target_link_libraries(voxalibera_tests
    PRIVATE
        juce::juce_audio_formats
        PkgConfig::ALSA
        Catch2::Catch2WithMain
    PUBLIC
        juce::juce_recommended_config_flags)

set_source_files_properties(${VOXALIBERA_TEST_SOURCES} PROPERTIES COMPILE_OPTIONS "${VOXALIBERA_WARNINGS}")

add_test(NAME voxalibera_tests COMMAND voxalibera_tests)
