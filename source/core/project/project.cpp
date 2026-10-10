#include "core/project/project.h"

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
