#pragma once

#include "core/media/peaks.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <cstdint>

struct ClipAppearance
{
	juce::String name;
	juce::Colour colour;
	const Peaks* peaks = nullptr;
	std::int64_t source_offset = 0;
	std::int64_t length = 0;
	double timeline_rate = 0.0;
	bool selected = false;
	bool recording = false;
};

void paint_clip(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ClipAppearance& clip);
