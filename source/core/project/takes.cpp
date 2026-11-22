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

	Clip trimmed(Clip clip, std::int64_t start, std::int64_t end)
	{
		const auto new_start = std::max(start, clip.start);
		const auto new_end = std::min(end, clip.end());
		const auto cut_left = new_start - clip.start;
		const auto cut_right = clip.end() - new_end;

		clip.start = new_start;
		clip.source_offset += cut_left;
		clip.length = new_end - new_start;
		clip.fade_in = cut_left > 0 ? 0 : std::min(clip.fade_in, clip.length);
		clip.fade_out = cut_right > 0 ? 0 : std::min(clip.fade_out, clip.length - clip.fade_in);
		return clip;
	}

	std::vector<TrackRange> merged(std::vector<TrackRange> ranges)
	{
		std::ranges::sort(ranges, [](const TrackRange& left, const TrackRange& right) {
			return std::pair(left.track.value, left.start) < std::pair(right.track.value, right.start);
		});

		std::vector<TrackRange> result;

		for (const auto& range : ranges) {
			if (!result.empty() && result.back().track == range.track && range.start <= result.back().end)
				result.back().end = std::max(result.back().end, range.end);
			else
				result.push_back(range);
		}

		return result;
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

std::vector<TrackRange> comp_ranges(const Project& project, const std::vector<ClipId>& clips)
{
	std::vector<TrackRange> ranges;

	for (const auto id : clips) {
		const auto* clip = project.find_clip(id);
		const auto* track = project.find_track_of(id);

		if (clip != nullptr && track != nullptr && has_takes(*track))
			ranges.push_back({track->id, clip->start, clip->end()});
	}

	return merged(std::move(ranges));
}

std::vector<Clip> takes_in_range(const Track& track, std::int64_t start, std::int64_t end)
{
	std::vector<Clip> result;

	for (const auto& clip : track.clips)
		if (overlaps(clip, start, end))
			result.push_back(trimmed(clip, start, end));

	return result;
}

void remove_range(Edit& edit, const Project& project, TrackId track_id, std::int64_t start, std::int64_t end)
{
	if (project.find_track(track_id) == nullptr || end <= start)
		return;

	split_all_at(edit, project, track_id, start);
	split_all_at(edit, project, track_id, end);

	std::vector<ClipId> inside;

	for (const auto& clip : project.find_track(track_id)->clips)
		if (start <= clip.start && clip.end() <= end)
			inside.push_back(clip.id);

	for (const auto id : inside)
		edit.remove_clip(id);

	compact_lanes(edit, project, track_id);
}

std::vector<ClipId> add_comp(Edit& edit, const Project& project, TrackId track_id, std::vector<Clip> clips)
{
	const auto* track = project.find_track(track_id);

	if (track == nullptr || clips.empty())
		return {};

	const auto start = std::ranges::min(clips, {}, &Clip::start).start;
	const auto end = std::ranges::max(clips, {}, &Clip::end).end();
	auto first_free_lane = 0;

	for (const auto& existing : track->clips)
		if (overlaps(existing, start, end))
			first_free_lane = std::max(first_free_lane, existing.lane + 1);

	std::vector<ClipId> added;

	for (auto& clip : clips) {
		clip.lane += first_free_lane;
		added.push_back(edit.add_clip(track_id, clip));
	}

	for (const auto& clip : clips)
		if (clip.active)
			comp_lane(edit, project, track_id, clip.lane, clip.start, clip.end());

	compact_lanes(edit, project, track_id);
	return added;
}

void compact_lanes(Edit& edit, const Project& project, TrackId track_id)
{
	const auto* track = project.find_track(track_id);

	if (track == nullptr)
		return;

	std::vector<int> used;

	for (const auto& clip : track->clips)
		used.push_back(clip.lane);

	std::ranges::sort(used);
	used.erase(std::unique(used.begin(), used.end()), used.end());

	std::vector<std::pair<ClipId, int>> moves;
	std::vector<ClipId> silent;

	for (const auto& clip : track->clips) {
		const auto lane = (int) (std::ranges::lower_bound(used, clip.lane) - used.begin());

		if (lane != clip.lane)
			moves.emplace_back(clip.id, lane);

		if (used.size() == 1 && !clip.active)
			silent.push_back(clip.id);
	}

	for (const auto& [id, lane] : moves)
		edit.set(id, &Clip::lane, lane);

	for (const auto id : silent)
		edit.set(id, &Clip::active, true);
}
