#include "core/project/model.h"

#include <algorithm>

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

Track* Project::find_track(TrackId id)
{
	const auto index = index_of(id);
	return index == NOT_FOUND ? nullptr : &tracks[(size_t) index];
}

const Track* Project::find_track(TrackId id) const
{
	const auto index = index_of(id);
	return index == NOT_FOUND ? nullptr : &tracks[(size_t) index];
}

int Project::index_of(TrackId id) const
{
	for (size_t position = 0; position < tracks.size(); ++position)
		if (tracks[position].id == id)
			return (int) position;

	return NOT_FOUND;
}

TrackId Project::allocate_track_id()
{
	return TrackId{next_id++};
}

Clip* Project::find_clip(ClipId id)
{
	for (auto& track : tracks)
		for (auto& clip : track.clips)
			if (clip.id == id)
				return &clip;

	return nullptr;
}

const Clip* Project::find_clip(ClipId id) const
{
	for (const auto& track : tracks)
		for (const auto& clip : track.clips)
			if (clip.id == id)
				return &clip;

	return nullptr;
}

Track* Project::find_track_of(ClipId id)
{
	for (auto& track : tracks)
		for (const auto& clip : track.clips)
			if (clip.id == id)
				return &track;

	return nullptr;
}

const Track* Project::find_track_of(ClipId id) const
{
	for (const auto& track : tracks)
		for (const auto& clip : track.clips)
			if (clip.id == id)
				return &track;

	return nullptr;
}

ClipId Project::allocate_clip_id()
{
	return ClipId{next_id++};
}
