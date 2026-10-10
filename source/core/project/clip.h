#pragma once

#include "core/project/ids.h"

#include <juce_core/juce_core.h>

#include <cstdint>

struct Clip
{
	ClipId id;
	juce::String file;
	std::int64_t start = 0;
	std::int64_t source_offset = 0;
	std::int64_t length = 0;
	float gain_db = 0.0f;
	int lane = 0;

	std::int64_t end() const { return start + length; }
};
