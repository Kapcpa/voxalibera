#include "core/project/model.h"

#include <algorithm>

namespace
{
	constexpr std::uint32_t MASTER_COLOUR = 0xff6c7078;
}

Track master_track()
{
	Track master;
	master.id = MASTER_TRACK;
	master.name = "Master";
	master.colour = MASTER_COLOUR;
	master.channels = TrackChannels::stereo;
	master.input = TrackInput{TrackInput::NONE};
	return master;
}

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
	if (id == MASTER_TRACK)
		return &master;

	const auto index = index_of(id);
	return index == NOT_FOUND ? nullptr : &tracks[(size_t) index];
}

const Track* Project::find_track(TrackId id) const
{
	if (id == MASTER_TRACK)
		return &master;

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

std::vector<Track*> Project::chain_owners()
{
	std::vector<Track*> owners;

	for (auto& track : tracks)
		owners.push_back(&track);

	owners.push_back(&master);
	return owners;
}

std::vector<const Track*> Project::chain_owners() const
{
	std::vector<const Track*> owners;

	for (const auto& track : tracks)
		owners.push_back(&track);

	owners.push_back(&master);
	return owners;
}

Plugin* Project::find_plugin(PluginId id)
{
	for (auto* track : chain_owners())
		for (auto& plugin : track->plugins)
			if (plugin.id == id)
				return &plugin;

	return nullptr;
}

const Plugin* Project::find_plugin(PluginId id) const
{
	for (const auto* track : chain_owners())
		for (const auto& plugin : track->plugins)
			if (plugin.id == id)
				return &plugin;

	return nullptr;
}

Track* Project::find_track_of(PluginId id)
{
	for (auto* track : chain_owners())
		for (const auto& plugin : track->plugins)
			if (plugin.id == id)
				return track;

	return nullptr;
}

ClipId Project::allocate_clip_id()
{
	return ClipId{next_id++};
}

PluginId Project::allocate_plugin_id()
{
	return PluginId{next_id++};
}

double timeline_rate(const Project& project, double device_rate)
{
	return project.sample_rate > 0.0 ? project.sample_rate : device_rate;
}

std::int64_t project_end(const Project& project)
{
	std::int64_t end = 0;

	for (const auto& track : project.tracks)
		for (const auto& clip : track.clips)
			end = std::max(end, clip.end());

	return end;
}
