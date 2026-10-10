#pragma once

#include "core/project/project.h"

#include <vector>

struct Selection
{
	bool contains(TrackId id) const;
	void select_only(TrackId id);
	void toggle(TrackId id);
	void clear();
	void forget_missing(const Project& project);

	std::vector<TrackId> tracks;
	int revision = 0;
};
