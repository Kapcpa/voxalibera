#pragma once

#include "core/project/ids.h"

#include <juce_core/juce_core.h>

#include <cstdint>

enum class TrackChannels
{
	mono,
	stereo,
};

struct TrackInput
{
	static constexpr int NONE = -1;

	int first_channel = 0;

	bool is_connected() const { return first_channel != NONE; }
	bool operator==(const TrackInput&) const = default;
};

struct Track
{
	TrackId id;
	juce::String name;
	std::uint32_t colour = 0;
	TrackChannels channels = TrackChannels::mono;
	TrackInput input;
	bool armed = false;
	bool mute = false;
	bool solo = false;
	float volume_db = 0.0f;
	float pan = 0.0f;
};

int channel_count(TrackChannels channels);
juce::String describe_channels(TrackChannels channels);
juce::String describe_input(const TrackInput& input, TrackChannels channels);
