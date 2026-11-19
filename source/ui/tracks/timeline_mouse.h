#pragma once

#include "core/app_context.h"
#include "ui/tracks/timeline_layout.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <optional>

enum class ClipZone
{
	body,
	fade_in,
	fade_out,
	gain,
	trim_left,
	trim_right
};

struct ClipGesture
{
	ClipZone zone = ClipZone::body;
	ClipId anchor;
	std::int64_t anchor_start = 0;
	double grab_beat = 0.0;
	int grab_track = 0;
	float gain_db = 0.0f;
	ClipMove move;
};

struct TimeSelectionGesture
{
	std::int64_t grab_tick = 0;
	bool selected = false;
};

struct CompGesture
{
	size_t track_index = 0;
	int lane = 0;
	std::int64_t grab_sample = 0;
	std::int64_t current_sample = 0;
};

class TimelineMouse
{
public:
	TimelineMouse(AppContext& context, const TimelineLayout& layout);

	void down(const juce::MouseEvent& event);
	void drag(const juce::MouseEvent& event);
	void up(const juce::MouseEvent& event);
	juce::MouseCursor cursor_at(juce::Point<int> position) const;
	const std::optional<ClipGesture>& gesture() const { return current; }
	const std::optional<CompGesture>& comp_gesture() const { return comp; }

private:
	ClipZone zone_at(juce::Point<int> position, const ClipUnder& clip) const;
	void select_track_at(const juce::MouseEvent& event);
	bool start_comp(const juce::MouseEvent& event);
	void finish_comp(const juce::MouseEvent& event);
	void drag_time_selection(const juce::MouseEvent& event);
	void drag_move(const juce::MouseEvent& event);
	void drag_gain(const juce::MouseEvent& event);
	void drag_fade_in(const juce::MouseEvent& event);
	void drag_fade_out(const juce::MouseEvent& event);
	void drag_trim_left(const juce::MouseEvent& event);
	void drag_trim_right(const juce::MouseEvent& event);

	AppContext& context;
	const TimelineLayout& layout;
	std::optional<ClipGesture> current;
	std::optional<TimeSelectionGesture> time_selection;
	std::optional<CompGesture> comp;
};
