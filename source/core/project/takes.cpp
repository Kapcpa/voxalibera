#include "core/project/takes.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace
{
	constexpr double EDGE_FADE_SECONDS = 0.005;

	bool overlaps(const Clip& clip, std::int64_t start, std::int64_t end)
	{
		return clip.start < end && start < clip.end();
	}

	bool continues(const Clip& left, const Clip& right)
	{
		return left.file == right.file && juce::exactlyEqual(left.gain_db, right.gain_db) && left.source_offset + left.length == right.source_offset;
	}

	void split_all_at(Edit& edit, const Project& project, TrackId track_id, std::int64_t position)
	{
		std::vector<ClipId> crossing;

		for (const auto& clip : project.find_track(track_id)->clips)
			if (clip.start < position && position < clip.end())
				crossing.push_back(clip.id);

		for (const auto id : crossing)
			split_clip(edit, project, id, position);
	}

	void set_fade(Edit& edit, const Clip& clip, std::int64_t Clip::* fade, std::int64_t Clip::* other_fade, std::int64_t wanted)
	{
		const auto value = std::clamp<std::int64_t>(wanted, 0, clip.length - clip.*other_fade);

		if (clip.*fade != value)
			edit.set(clip.id, fade, value);
	}

	void soften_edge(Edit& edit, const Project& project, TrackId track_id, std::int64_t position)
	{
		const Clip* left = nullptr;
		const Clip* right = nullptr;

		for (const auto& clip : project.find_track(track_id)->clips) {
			if (!clip.active)
				continue;

			if (clip.end() == position)
				left = &clip;

			if (clip.start == position)
				right = &clip;
		}

		const auto edge_fade = (std::int64_t) std::llround(project.sample_rate * EDGE_FADE_SECONDS);
		const auto seamless = left != nullptr && right != nullptr && continues(*left, *right);

		if (left != nullptr)
			set_fade(edit, *left, &Clip::fade_out, &Clip::fade_in, seamless ? 0 : std::max(left->fade_out, edge_fade));

		if (right != nullptr)
			set_fade(edit, *right, &Clip::fade_in, &Clip::fade_out, seamless ? 0 : std::max(right->fade_in, edge_fade));
	}

	void comp_range(Edit& edit, const Project& project, TrackId track_id, int lane, std::int64_t start, std::int64_t end)
	{
		split_all_at(edit, project, track_id, start);
		split_all_at(edit, project, track_id, end);

		std::vector<std::pair<ClipId, bool>> changes;

		for (const auto& clip : project.find_track(track_id)->clips)
			if (start <= clip.start && clip.end() <= end && clip.active != (clip.lane == lane))
				changes.emplace_back(clip.id, clip.lane == lane);

		for (const auto& [id, active] : changes)
			edit.set(id, &Clip::active, active);

		soften_edge(edit, project, track_id, start);
		soften_edge(edit, project, track_id, end);
	}
}

int lane_count(const Track& track)
{
	auto highest = 0;

	for (const auto& clip : track.clips)
		highest = std::max(highest, clip.lane);

	return highest + 1;
}

bool has_takes(const Track& track)
{
	return lane_count(track) > 1;
}

bool shows_lanes(const Track& track)
{
	return track.expanded && has_takes(track);
}

void keep_comp(Edit& edit, const Project& project, TrackId track_id)
{
	const auto* track = project.find_track(track_id);

	if (track == nullptr)
		return;

	std::vector<ClipId> inactive;
	std::vector<ClipId> moved;

	for (const auto& clip : track->clips) {
		if (!clip.active)
			inactive.push_back(clip.id);
		else if (clip.lane != 0)
			moved.push_back(clip.id);
	}

	for (const auto id : inactive)
		edit.remove_clip(id);

	for (const auto id : moved)
		edit.set(id, &Clip::lane, 0);

	if (track->expanded)
		edit.set(track_id, &Track::expanded, false);
}

ClipId split_clip(Edit& edit, const Project& project, ClipId id, std::int64_t position)
{
	const auto* clip = project.find_clip(id);

	if (clip == nullptr || position <= clip->start || clip->end() <= position)
		return {};

	const auto cut = position - clip->start;
	const auto track_id = project.find_track_of(id)->id;

	auto right = *clip;
	right.start = position;
	right.source_offset += cut;
	right.length -= cut;
	right.fade_in = 0;
	right.fade_out = std::min(clip->fade_out, clip->length - cut);

	edit.set(id, &Clip::length, cut);
	edit.set(id, &Clip::fade_in, std::min(clip->fade_in, cut));
	edit.set(id, &Clip::fade_out, (std::int64_t) 0);

	return edit.add_clip(track_id, right);
}

void comp_lane(Edit& edit, const Project& project, TrackId track_id, int lane, std::int64_t start, std::int64_t end)
{
	const auto* track = project.find_track(track_id);

	if (track == nullptr || end <= start)
		return;

	std::vector<std::pair<std::int64_t, std::int64_t>> covered;

	for (const auto& clip : track->clips)
		if (clip.lane == lane && overlaps(clip, start, end))
			covered.emplace_back(std::max(start, clip.start), std::min(end, clip.end()));

	for (const auto& [from, to] : covered)
		comp_range(edit, project, track_id, lane, from, to);
}

ClipId add_take(Edit& edit, const Project& project, TrackId track_id, Clip clip)
{
	const auto* track = project.find_track(track_id);

	if (track == nullptr)
		return {};

	clip.lane = 0;
	clip.active = true;

	for (const auto& existing : track->clips)
		if (overlaps(existing, clip.start, clip.end()))
			clip.lane = std::max(clip.lane, existing.lane + 1);

	const auto id = edit.add_clip(track_id, clip);
	comp_lane(edit, project, track_id, clip.lane, clip.start, clip.end());
	return id;
}
