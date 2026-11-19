#pragma once

#include "core/app_context.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <optional>
#include <vector>

struct TimelineViewport
{
	static constexpr double MIN_PIXELS_PER_BEAT = 2.0;
	static constexpr double MAX_PIXELS_PER_BEAT = 400.0;
	static constexpr int TRACK_HEIGHT = 64;
	static constexpr int LANE_HEIGHT = 36;

	double beat_to_x(double beat) const { return (beat - first_beat) * pixels_per_beat; }
	double x_to_beat(double x) const { return first_beat + x / pixels_per_beat; }
	void zoom_around(double x, double factor);
	void scroll_beats(double beats);

	double pixels_per_beat = 24.0;
	double first_beat = 0.0;
	int scroll_y = 0;
	int revision = 0;
};

struct ClipUnder
{
	ClipId id;
	juce::Rectangle<float> area;
};

struct LaneUnder
{
	size_t track_index = 0;
	int lane = 0;
};

int track_height(const Track& track);
int tracks_height(const Project& project);

struct TimelineLayout
{
	double sample_rate() const;
	std::int64_t grid_step() const;
	double snap(double beat, const juce::ModifierKeys& modifiers) const;
	std::int64_t tick_at(int x, const juce::ModifierKeys& modifiers) const;
	float tick_to_x(std::int64_t tick) const;
	std::vector<GridLine> visible_grid_lines(int width) const;
	double sample_to_x(std::int64_t sample) const;
	std::int64_t sample_at(int x, const juce::ModifierKeys& modifiers) const;
	int track_top(size_t track_index) const;
	int lane_top(size_t track_index, int lane) const;
	juce::Rectangle<float> clip_area(size_t track_index, std::int64_t start, std::int64_t length) const;
	juce::Rectangle<float> lane_clip_area(size_t track_index, int lane, std::int64_t start, std::int64_t length) const;
	std::optional<ClipUnder> clip_at(juce::Point<int> position) const;
	std::optional<ClipUnder> lane_clip_at(juce::Point<int> position) const;
	std::optional<LaneUnder> lane_at(int y) const;
	int track_index_at(int y) const;

	const AppContext& context;
	const TimelineViewport& viewport;
};
