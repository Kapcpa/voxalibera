#include "core/project/track.h"

int channel_count(TrackChannels channels)
{
	return channels == TrackChannels::stereo ? 2 : 1;
}

juce::String describe_channels(TrackChannels channels)
{
	return channels == TrackChannels::stereo ? "Stereo" : "Mono";
}

juce::String describe_input(const TrackInput& input, TrackChannels channels)
{
	if (!input.is_connected())
		return "No input";

	const auto first = input.first_channel + 1;
	return channels == TrackChannels::stereo ? "In " + juce::String(first) + "+" + juce::String(first + 1) : "In " + juce::String(first);
}
