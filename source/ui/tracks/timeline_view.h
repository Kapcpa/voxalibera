#pragma once

#include "core/app_context.h"
#include "core/media/audio_import.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <optional>

struct TimelineViewport
{
	static constexpr double MIN_PIXELS_PER_BEAT = 2.0;
	static constexpr double MAX_PIXELS_PER_BEAT = 400.0;
	static constexpr int TRACK_HEIGHT = 64;

	double beat_to_x(double beat) const { return (beat - first_beat) * pixels_per_beat; }
	double x_to_beat(double x) const { return first_beat + x / pixels_per_beat; }
	void zoom_around(double x, double factor);
	void scroll_beats(double beats);

	double pixels_per_beat = 24.0;
	double first_beat = 0.0;
	int scroll_y = 0;
	int revision = 0;
};

class TimelineView : public juce::Component, public juce::FileDragAndDropTarget
{
public:
	TimelineView(AppContext& context, const TimelineViewport& viewport);

	void paint(juce::Graphics& graphics) override;
	void mouseDown(const juce::MouseEvent& event) override;
	void mouseDrag(const juce::MouseEvent& event) override;
	void mouseUp(const juce::MouseEvent& event) override;

	bool isInterestedInFileDrag(const juce::StringArray& files) override;
	void fileDragMove(const juce::StringArray& files, int x, int y) override;
	void fileDragExit(const juce::StringArray& files) override;
	void filesDropped(const juce::StringArray& files, int x, int y) override;

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
	ImportPlace import_place_at(int x, int y) const;
	void paint_clips(juce::Graphics& graphics);
	void paint_live_takes(juce::Graphics& graphics);
	void paint_lanes(juce::Graphics& graphics);
	void paint_grid(juce::Graphics& graphics);
	void paint_playhead(juce::Graphics& graphics);
	void paint_drop_place(juce::Graphics& graphics);

	AppContext& context;
	const TimelineViewport& viewport;
	std::optional<ClipDrag> drag;
	std::optional<ImportPlace> drop_place;
};
