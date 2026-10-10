#pragma once

#include "core/app_context.h"
#include "ui/tracks/ruler_view.h"
#include "ui/tracks/timeline_view.h"
#include "ui/tracks/track_header.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <map>
#include <memory>

class TrackAreaView : public juce::Component
{
public:
	explicit TrackAreaView(AppContext& context);

	void update();
	void paint(juce::Graphics& graphics) override;
	void resized() override;
	void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

	std::function<void()> on_add_track;

private:
	void sync_headers();
	void layout_headers();
	void follow_playhead();
	void clamp_vertical_scroll();
	int content_height() const;

	AppContext& context;
	TimelineViewport viewport;
	RulerView ruler;
	TimelineView timeline;
	juce::Component header_column;
	juce::TextButton add_track_button{"+ Track"};
	std::map<TrackId, std::unique_ptr<TrackHeader>> headers;
	int shown_project_revision = -1;
	int shown_selection_revision = -1;
	int shown_viewport_revision = -1;
	std::int64_t shown_position = -1;
	int shown_peaks_revision = -1;
};
