#pragma once

#include "core/project/model.h"

#include <vector>

struct Selection
{
	bool contains(TrackId id) const;
	bool contains(ClipId id) const;
	void select_only(TrackId id);
	void select_only(ClipId id);
	void toggle(TrackId id);
	void toggle(ClipId id);
	void clear();
	void forget_missing(const Project& project);

	std::vector<TrackId> tracks;
	std::vector<ClipId> clips;
	int revision = 0;
};
