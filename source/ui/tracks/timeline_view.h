#pragma once

#include "core/app_context.h"
#include "core/media/audio_import.h"
#include "ui/tracks/timeline_layout.h"
#include "ui/tracks/timeline_mouse.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <optional>

class TimelineView : public juce::Component, public juce::FileDragAndDropTarget
{
public:
	TimelineView(AppContext& context, const TimelineViewport& viewport);

	void paint(juce::Graphics& graphics) override;
	void mouseDown(const juce::MouseEvent& event) override;
	void mouseDrag(const juce::MouseEvent& event) override;
	void mouseUp(const juce::MouseEvent& event) override;
	void mouseMove(const juce::MouseEvent& event) override;

	bool isInterestedInFileDrag(const juce::StringArray& files) override;
	void fileDragMove(const juce::StringArray& files, int x, int y) override;
	void fileDragExit(const juce::StringArray& files) override;
	void filesDropped(const juce::StringArray& files, int x, int y) override;

	double playhead_beat() const;

private:
	ImportPlace import_place_at(int x, int y) const;
	void paint_clips(juce::Graphics& graphics);
	void paint_live_takes(juce::Graphics& graphics);
	void paint_lanes(juce::Graphics& graphics);
	void paint_grid(juce::Graphics& graphics);
	void paint_playhead(juce::Graphics& graphics);
	void paint_drop_place(juce::Graphics& graphics);

	AppContext& context;
	const TimelineViewport& viewport;
	TimelineLayout layout;
	TimelineMouse mouse;
	std::optional<ImportPlace> drop_place;
};
