#pragma once

#include "core/app_context.h"
#include "ui/tracks/timeline_view.h"

#include <juce_gui_basics/juce_gui_basics.h>

class RulerView : public juce::Component
{
public:
	static constexpr int HEIGHT = 28;

	RulerView(AppContext& context, const TimelineViewport& viewport);

	void paint(juce::Graphics& graphics) override;
	void mouseDown(const juce::MouseEvent& event) override;
	void mouseDrag(const juce::MouseEvent& event) override;

private:
	void paint_bar_labels(juce::Graphics& graphics);
	void paint_grid_name(juce::Graphics& graphics);
	void locate_to(const juce::MouseEvent& event);

	AppContext& context;
	const TimelineViewport& viewport;
	TimelineLayout layout;
};
