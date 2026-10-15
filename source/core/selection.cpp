#include "core/selection.h"

#include <algorithm>

bool Selection::contains(TrackId id) const
{
	return std::find(tracks.begin(), tracks.end(), id) != tracks.end();
}

bool Selection::contains(ClipId id) const
{
	return std::find(clips.begin(), clips.end(), id) != clips.end();
}

void Selection::select_only(TrackId id)
{
	tracks = {id};
	clips.clear();
	clear_chain();
}

void Selection::select_only(ClipId id)
{
	clips = {id};
	clear_chain();
}

void Selection::select_only(PluginId id)
{
	clips.clear();
	gap.reset();
	plugin = id;
	++revision;
}

void Selection::select_gap(size_t position)
{
	clips.clear();
	plugin = {};
	gap = position;
	++revision;
}

void Selection::clear_chain()
{
	plugin = {};
	gap.reset();
	++revision;
}

void Selection::toggle(TrackId id)
{
	if (contains(id))
		std::erase(tracks, id);
	else
		tracks.push_back(id);

	clear_chain();
}

void Selection::toggle(ClipId id)
{
	if (contains(id))
		std::erase(clips, id);
	else
		clips.push_back(id);

	clear_chain();
}

void Selection::clear()
{
	tracks.clear();
	clips.clear();
	clear_chain();
}

void Selection::forget_missing(const Project& project)
{
	const auto removed_tracks = std::erase_if(tracks, [&](TrackId id) { return project.find_track(id) == nullptr; });
	const auto removed_clips = std::erase_if(clips, [&](ClipId id) { return project.find_clip(id) == nullptr; });

	if (removed_tracks + removed_clips > 0)
		clear_chain();

	if (plugin.is_valid() && project.find_plugin(plugin) == nullptr)
		clear_chain();
}
