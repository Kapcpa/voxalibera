#pragma once

#include "core/project/model.h"
#include <juce_core/juce_core.h>

class AppContext;

struct ImportPlace
{
	std::optional<TrackId> track;
	std::int64_t start = 0;
};

juce::String importable_audio_wildcard();
bool is_importable_audio(const juce::File& file);
bool import_audio_file(AppContext& context, const juce::File& file, ImportPlace import_place = {});
