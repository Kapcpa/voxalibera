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
	++revision;
}

void Selection::select_only(ClipId id)
{
	clips = {id};
	++revision;
}

void Selection::toggle(TrackId id)
{
	if (contains(id))
		std::erase(tracks, id);
	else
		tracks.push_back(id);

	++revision;
}

void Selection::toggle(ClipId id)
{
	if (contains(id))
		std::erase(clips, id);
	else
		clips.push_back(id);

	++revision;
}

void Selection::clear()
{
	tracks.clear();
	clips.clear();
	++revision;
}

void Selection::forget_missing(const Project& project)
{
	const auto removed_tracks = std::erase_if(tracks, [&](TrackId id) { return project.find_track(id) == nullptr; });
	const auto removed_clips = std::erase_if(clips, [&](ClipId id) { return project.find_clip(id) == nullptr; });

	if (removed_tracks + removed_clips > 0)
		++revision;
}
