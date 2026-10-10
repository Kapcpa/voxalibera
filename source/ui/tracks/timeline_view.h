#pragma once

#include "core/app_context.h"
#include "ui/tracks/timeline_viewport.h"

#include <juce_gui_basics/juce_gui_basics.h>

class TimelineView : public juce::Component
{
public:
	TimelineView(AppContext& context, const TimelineViewport& viewport);

	void paint(juce::Graphics& graphics) override;
	void mouseDown(const juce::MouseEvent& event) override;

	double playhead_beat() const;

private:
	void paint_lanes(juce::Graphics& graphics);
	void paint_grid(juce::Graphics& graphics);
	void paint_playhead(juce::Graphics& graphics);

	AppContext& context;
	const TimelineViewport& viewport;
};
