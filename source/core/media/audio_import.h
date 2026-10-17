#pragma once

#include <juce_core/juce_core.h>

class AppContext;

juce::String importable_audio_wildcard();
bool import_audio_file(AppContext& context, const juce::File& file);
