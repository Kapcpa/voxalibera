#include "ui/tracks/timeline_layout.h"

#include "core/project/takes.h"

#include <algorithm>
#include <cmath>

namespace
{
	constexpr float CLIP_MARGIN = 3.0f;
	constexpr double MIN_GRID_LINE_SPACING = 6.0;
}

int track_height(const Track& track)
{
	return TimelineViewport::TRACK_HEIGHT + (shows_lanes(track) ? lane_count(track) * TimelineViewport::LANE_HEIGHT : 0);
}

int tracks_height(const Project& project)
{
	auto height = 0;

	for (const auto& track : project.tracks)
		height += track_height(track);

	return height;
}

void TimelineViewport::zoom_around(double x, double factor)
{
	const auto anchor_beat = x_to_beat(x);
	pixels_per_beat = std::clamp(pixels_per_beat * factor, MIN_PIXELS_PER_BEAT, MAX_PIXELS_PER_BEAT);
	first_beat = std::max(0.0, anchor_beat - x / pixels_per_beat);
	++revision;
}

void TimelineViewport::scroll_beats(double beats)
{
	first_beat = std::max(0.0, first_beat + beats);
	++revision;
}

double TimelineLayout::sample_rate() const
{
	return timeline_rate(context.project, context.engine.sample_rate.load());
}

std::int64_t TimelineLayout::grid_step() const
{
	return grid_step_ticks(context.project.grid, context.project.tempo.signature, viewport.pixels_per_beat);
}

double TimelineLayout::snap(double beat, const juce::ModifierKeys& modifiers) const
{
	if (!context.project.grid.snap || modifiers.isCommandDown())
		return beat;

	return snap_to_grid(beat, grid_step(), context.project.tempo.signature);
}

std::int64_t TimelineLayout::tick_at(int x, const juce::ModifierKeys& modifiers) const
{
	return std::llround(std::max(0.0, snap(viewport.x_to_beat(x), modifiers)) * TICKS_PER_BEAT);
}

float TimelineLayout::tick_to_x(std::int64_t tick) const
{
	return (float) viewport.beat_to_x((double) tick / TICKS_PER_BEAT);
}

std::vector<GridLine> TimelineLayout::visible_grid_lines(int width) const
{
	const auto& signature = context.project.tempo.signature;
	const auto step = grid_step();
	const auto pixels_per_step = (double) step / TICKS_PER_BEAT * viewport.pixels_per_beat;
	const auto drawn_step = pixels_per_step >= MIN_GRID_LINE_SPACING ? step : grid_step_ticks(Grid{}, signature, viewport.pixels_per_beat);

	return grid_lines(viewport.first_beat, viewport.x_to_beat(width), drawn_step, signature);
}

double TimelineLayout::sample_to_x(std::int64_t sample) const
{
	const auto rate = sample_rate();
	return rate > 0.0 ? viewport.beat_to_x((double) sample / samples_per_beat(context.project.tempo, rate)) : 0.0;
}

std::int64_t TimelineLayout::sample_at(int x, const juce::ModifierKeys& modifiers) const
{
	return std::llround(std::max(0.0, snap(viewport.x_to_beat(x), modifiers)) * samples_per_beat(context.project.tempo, sample_rate()));
}

int TimelineLayout::track_top(size_t track_index) const
{
	const auto& tracks = context.project.tracks;
	auto top = -viewport.scroll_y;

	for (size_t index = 0; index < std::min(track_index, tracks.size()); ++index)
		top += track_height(tracks[index]);

	return top + (int) (track_index - std::min(track_index, tracks.size())) * TimelineViewport::TRACK_HEIGHT;
}

int TimelineLayout::lane_top(size_t track_index, int lane) const
{
	return track_top(track_index) + TimelineViewport::TRACK_HEIGHT + lane * TimelineViewport::LANE_HEIGHT;
}

juce::Rectangle<float> TimelineLayout::clip_area(size_t track_index, std::int64_t start, std::int64_t length) const
{
	const auto top = (float) track_top(track_index) + CLIP_MARGIN;
	const auto left = (float) sample_to_x(start);
	return {left, top, (float) sample_to_x(start + length) - left, (float) TimelineViewport::TRACK_HEIGHT - 2.0f * CLIP_MARGIN};
}

juce::Rectangle<float> TimelineLayout::lane_clip_area(size_t track_index, int lane, std::int64_t start, std::int64_t length) const
{
	const auto top = (float) lane_top(track_index, lane) + CLIP_MARGIN;
	const auto left = (float) sample_to_x(start);
	return {left, top, (float) sample_to_x(start + length) - left, (float) TimelineViewport::LANE_HEIGHT - 2.0f * CLIP_MARGIN};
}

std::optional<ClipUnder> TimelineLayout::clip_at(juce::Point<int> position) const
{
	const auto& tracks = context.project.tracks;

	for (size_t index = 0; index < tracks.size(); ++index)
		for (const auto& clip : tracks[index].clips)
			if (const auto area = clip_area(index, clip.start, clip.length); clip.active && area.contains(position.toFloat()))
				return ClipUnder{clip.id, area};

	return std::nullopt;
}

std::optional<ClipUnder> TimelineLayout::lane_clip_at(juce::Point<int> position) const
{
	const auto lane = lane_at(position.y);

	if (!lane)
		return std::nullopt;

	for (const auto& clip : context.project.tracks[lane->track_index].clips)
		if (const auto area = lane_clip_area(lane->track_index, clip.lane, clip.start, clip.length); clip.lane == lane->lane && area.contains(position.toFloat()))
			return ClipUnder{clip.id, area};

	return std::nullopt;
}

std::optional<LaneUnder> TimelineLayout::lane_at(int y) const
{
	const auto index = track_index_at(y);
	const auto& tracks = context.project.tracks;

	if (index < 0 || index >= (int) tracks.size() || !shows_lanes(tracks[(size_t) index]))
		return std::nullopt;

	const auto below_comp = y - track_top((size_t) index) - TimelineViewport::TRACK_HEIGHT;

	if (below_comp < 0)
		return std::nullopt;

	return LaneUnder{(size_t) index, below_comp / TimelineViewport::LANE_HEIGHT};
}

int TimelineLayout::track_index_at(int y) const
{
	const auto& tracks = context.project.tracks;
	auto bottom = -viewport.scroll_y;

	if (y < bottom)
		return -1;

	for (size_t index = 0; index < tracks.size(); ++index) {
		bottom += track_height(tracks[index]);

		if (y < bottom)
			return (int) index;
	}

	return (int) tracks.size() + (y - bottom) / TimelineViewport::TRACK_HEIGHT;
}
