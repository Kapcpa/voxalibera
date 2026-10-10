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

bool any_solo(const std::vector<Track>& tracks)
{
	return std::any_of(tracks.begin(), tracks.end(), [](const Track& track) { return track.solo; });
}

bool is_audible(const Track& track, bool solo_active)
{
	return !track.mute && (!solo_active || track.solo);
}
