#pragma once

#include "core/project/model.h"
#include "core/selection.h"

#include <vector>

struct CopiedClip
{
	TrackId track;
	Clip clip;
};

std::vector<CopiedClip> copies_of(const Selection& selection, const Project& project);

struct Clipboard
{
	void copy(const Selection& selection, const Project& project);
	void clear();

	std::vector<CopiedClip> clips;
};
