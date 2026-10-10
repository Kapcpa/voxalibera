#pragma once

#include "core/app_context.h"
#include "ui/tracks/timeline_viewport.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <optional>

class TimelineView : public juce::Component
{
public:
	TimelineView(AppContext& context, const TimelineViewport& viewport);

	void paint(juce::Graphics& graphics) override;
	void mouseDown(const juce::MouseEvent& event) override;
	void mouseDrag(const juce::MouseEvent& event) override;
	void mouseUp(const juce::MouseEvent& event) override;

	double playhead_beat() const;

private:
	struct ClipDrag
	{
		ClipId anchor;
		std::int64_t anchor_start = 0;
		double grab_beat = 0.0;
		int grab_track = 0;
		ClipMove move;
	};

	double sample_rate() const;
	double sample_to_x(std::int64_t sample) const;
	juce::Rectangle<float> lane_area(size_t track_index) const;
	juce::Rectangle<float> clip_area(size_t track_index, std::int64_t start, std::int64_t length) const;
	std::optional<ClipId> clip_at(juce::Point<int> position) const;
	int track_index_at(int y) const;
	void paint_clips(juce::Graphics& graphics);
	void paint_live_takes(juce::Graphics& graphics);
	void paint_lanes(juce::Graphics& graphics);
	void paint_grid(juce::Graphics& graphics);
	void paint_playhead(juce::Graphics& graphics);

	AppContext& context;
	const TimelineViewport& viewport;
	std::optional<ClipDrag> drag;
};
