#pragma once

#include <juce_core/juce_core.h>

constexpr auto AUDIO_FOLDER = "audio";

juce::File new_unsaved_project_folder();
bool is_unsaved_project_folder(const juce::File& folder);
juce::File next_take_file(const juce::File& project_folder, const juce::String& track_name);
juce::String relative_media_path(const juce::File& project_folder, const juce::File& file);
juce::String media_display_name(const juce::String& relative_path);
