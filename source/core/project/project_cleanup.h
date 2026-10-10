#pragma once

#include "core/project/model.h"

#include <juce_core/juce_core.h>

constexpr juce::int64 UNUSED_HINT_BYTES = 100 * 1024 * 1024;

juce::Array<juce::File> unused_media(const Project& project);
juce::int64 total_size(const juce::Array<juce::File>& files);
juce::String describe_size(juce::int64 bytes);
int delete_media(const Project& project, const juce::Array<juce::File>& files);

bool should_hint_cleanup(const juce::File& project_file, juce::int64 unused_bytes);
void remember_cleanup_hint(const juce::File& project_file, juce::int64 unused_bytes);
