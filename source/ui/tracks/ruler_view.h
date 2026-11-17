#pragma once

#include "core/app_context.h"
#include "ui/tracks/timeline_view.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <optional>

class RulerView : public juce::Component
{
public:
	static constexpr int HEIGHT = 28;

	RulerView(AppContext& context, const TimelineViewport& viewport);

	void paint(juce::Graphics& graphics) override;
	void mouseDown(const juce::MouseEvent& event) override;
	void mouseDrag(const juce::MouseEvent& event) override;
	void mouseUp(const juce::MouseEvent& event) override;
	void mouseMove(const juce::MouseEvent& event) override;

private:
	enum class RulerZone
	{
		select,
		range_start,
		range_end,
	};

	struct RulerGesture
	{
		RulerZone zone = RulerZone::select;
		LoopRange original;
		std::int64_t grab_tick = 0;
		bool selected = false;
	};

	void paint_loop_range(juce::Graphics& graphics);
	void paint_bar_labels(juce::Graphics& graphics);
	void paint_grid_name(juce::Graphics& graphics);
	RulerZone zone_at(int x) const;
	void drag_select(const juce::MouseEvent& event);
	void drag_edge(const juce::MouseEvent& event);

	AppContext& context;
	const TimelineViewport& viewport;
	TimelineLayout layout;
	std::optional<RulerGesture> gesture;
};
