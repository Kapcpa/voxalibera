#pragma once

#include "core/project/model.h"
#include "core/selection.h"

#include <vector>

struct Clipboard
{
	struct CopiedClip
	{
		TrackId track;
		Clip clip;
	};

	void copy(const Selection& selection, const Project& project);
	void clear();

	std::vector<CopiedClip> clips;
};
