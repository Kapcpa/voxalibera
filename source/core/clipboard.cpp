#include "core/clipboard.h"

#include <algorithm>

void Clipboard::copy(const Selection& selection, const Project& project)
{
	std::vector<CopiedClip> copied;

	for (const auto id : selection.clips) {
		const auto* clip = project.find_clip(id);
		const auto* track = project.find_track_of(id);

		if (clip == nullptr || track == nullptr)
			continue;

		copied.push_back({track->id, *clip});
	}

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
