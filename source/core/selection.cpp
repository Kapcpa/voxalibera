#include "core/selection.h"

#include <algorithm>

bool Selection::contains(TrackId id) const
{
	return std::find(tracks.begin(), tracks.end(), id) != tracks.end();
}

void Selection::select_only(TrackId id)
{
	tracks = {id};
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

void Selection::clear()
{
	tracks.clear();
	++revision;
}

void Selection::forget_missing(const Project& project)
{
	if (std::erase_if(tracks, [&](TrackId id) { return project.find_track(id) == nullptr; }) > 0)
		++revision;
}
