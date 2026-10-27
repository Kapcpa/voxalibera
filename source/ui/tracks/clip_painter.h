#pragma once

#include "core/media/peaks.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <cstdint>

constexpr float CLIP_HEADER_HEIGHT = 15.0f;

struct ClipAppearance
{
	juce::String name;
	juce::Colour colour;
	const Peaks* peaks = nullptr;
	std::int64_t source_offset = 0;
	std::int64_t length = 0;
	double timeline_rate = 0.0;
	int track_channels = 2;
	bool selected = false;
	bool recording = false;
	float gain_db = 0.0f;
	std::int64_t fade_in = 0;
	std::int64_t fade_out = 0;
};

struct FadeHandles
{
	float fade_in_x = 0.0f;
	float fade_out_x = 0.0f;
	float y = 0.0f;
};

FadeHandles fade_handles(juce::Rectangle<float> bounds, std::int64_t length, std::int64_t fade_in, std::int64_t fade_out);
void paint_clip(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ClipAppearance& clip);
