#include "ui/tracks/timeline_view.h"

#include "ui/theme.h"

#include <cmath>

namespace
{
	constexpr double MIN_BEAT_LINE_SPACING = 6.0;
	constexpr float PLAYHEAD_WIDTH = 1.5f;
	constexpr float HINT_FONT_SIZE = 14.0f;
	constexpr auto EMPTY_HINT = "No tracks yet. Press Ctrl+T or + Track to add one.";
}

TimelineView::TimelineView(AppContext& context, const TimelineViewport& viewport) : context(context), viewport(viewport)
{
}

double TimelineView::playhead_beat() const
{
	const auto rate = context.engine.sample_rate.load();

	if (rate <= 0.0)
		return 0.0;

	return (double) context.engine.transport.position() / samples_per_beat(context.project.tempo, rate);
}

void TimelineView::paint(juce::Graphics& graphics)
{
	graphics.fillAll(theme::WINDOW_BACKGROUND);
	paint_lanes(graphics);
	paint_grid(graphics);
	paint_playhead(graphics);

	if (context.project.tracks.empty()) {
		graphics.setColour(theme::DIM_TEXT);
		graphics.setFont(juce::FontOptions(HINT_FONT_SIZE));
		graphics.drawText(EMPTY_HINT, getLocalBounds(), juce::Justification::centred);
	}
}

void TimelineView::paint_lanes(juce::Graphics& graphics)
{
	const auto& tracks = context.project.tracks;

	for (size_t position = 0; position < tracks.size(); ++position) {
		const auto y = (int) position * TimelineViewport::TRACK_HEIGHT - viewport.scroll_y;
		const auto lane = juce::Rectangle<int>(0, y, getWidth(), TimelineViewport::TRACK_HEIGHT);

		if (context.selection.contains(tracks[position].id)) {
			graphics.setColour(theme::LANE_SELECTED);
			graphics.fillRect(lane);
		}

		graphics.setColour(theme::GRID_BEAT);
		graphics.drawHorizontalLine(lane.getBottom() - 1, 0.0f, (float) getWidth());
	}
}

void TimelineView::paint_grid(juce::Graphics& graphics)
{
	const auto beats_per_bar = context.project.tempo.signature.beats_per_bar;
	const auto first_beat = (int) std::floor(viewport.first_beat);
	const auto last_beat = (int) std::ceil(viewport.x_to_beat(getWidth()));

	for (auto beat = first_beat; beat <= last_beat; ++beat) {
		const auto is_bar = beat % beats_per_bar == 0;

		if (!is_bar && viewport.pixels_per_beat < MIN_BEAT_LINE_SPACING)
			continue;

		graphics.setColour(is_bar ? theme::GRID_BAR : theme::GRID_BEAT);
		graphics.drawVerticalLine((int) viewport.beat_to_x(beat), 0.0f, (float) getHeight());
	}
}

void TimelineView::paint_playhead(juce::Graphics& graphics)
{
	const auto x = (float) viewport.beat_to_x(playhead_beat());

	if (x < 0.0f || x > (float) getWidth())
		return;

	graphics.setColour(theme::ACCENT);
	graphics.fillRect(x, 0.0f, PLAYHEAD_WIDTH, (float) getHeight());
}

void TimelineView::mouseDown(const juce::MouseEvent& event)
{
	const auto index = (event.y + viewport.scroll_y) / TimelineViewport::TRACK_HEIGHT;
	const auto& tracks = context.project.tracks;

	if (index < 0 || index >= (int) tracks.size()) {
		context.selection.clear();
		return;
	}

	context.selection.select_only(tracks[(size_t) index].id);
}
