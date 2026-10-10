#pragma once

#include "core/app_context.h"
#include "ui/tracks/timeline_viewport.h"

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
	void locate_to(int x);

	AppContext& context;
	const TimelineViewport& viewport;
};
