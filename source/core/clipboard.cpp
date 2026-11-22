#include "core/clipboard.h"

#include "core/project/takes.h"

#include <algorithm>

std::vector<CopiedClip> copies_of(const Selection& selection, const Project& project)
{
	std::vector<CopiedClip> copies;

	for (const auto id : selection.clips) {
		const auto* clip = project.find_clip(id);
		const auto* track = project.find_track_of(id);

		if (clip != nullptr && track != nullptr && !has_takes(*track))
			copies.push_back({track->id, *clip});
	}

	for (const auto& range : comp_ranges(project, selection.clips))
		for (const auto& clip : takes_in_range(*project.find_track(range.track), range.start, range.end))
			copies.push_back({range.track, clip});

	return copies;
}

void Clipboard::copy(const Selection& selection, const Project& project)
{
	auto copied = copies_of(selection, project);

	if (copied.empty())
		return;

	const auto earliest_start = std::ranges::min(copied, {}, [](const CopiedClip& entry) { return entry.clip.start; }).clip.start;

	for (auto& entry : copied)
		entry.clip.start -= earliest_start;

	clips = std::move(copied);
}

void Clipboard::clear()
{
	clips.clear();
}
