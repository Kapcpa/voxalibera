#include "ui/tracks/track_area_view.h"

#include "ui/theme.h"

#include <cmath>

namespace
{
	constexpr double WHEEL_SCROLL_PIXELS = 240.0;
	constexpr double WHEEL_ZOOM_SPEED = 2.0;
	constexpr double FOLLOW_RIGHT_EDGE = 0.9;
	constexpr double FOLLOW_LEFT_MARGIN = 0.1;
	constexpr int ADD_BUTTON_MARGIN = 4;
}

TrackAreaView::TrackAreaView(AppContext& context) : context(context), ruler(context, viewport), timeline(context, viewport)
{
	add_track_button.setWantsKeyboardFocus(false);
	add_track_button.onClick = [this] {
		if (on_add_track)
			on_add_track();
	};

	addAndMakeVisible(ruler);
	addAndMakeVisible(timeline);
	addAndMakeVisible(header_column);
	addAndMakeVisible(add_track_button);
}

void TrackAreaView::update()
{
	follow_playhead();

	const auto structure_changed = context.project.revision != shown_project_revision || context.selection.revision != shown_selection_revision;
	const auto view_changed = viewport.revision != shown_viewport_revision;

	if (structure_changed)
		sync_headers();

	if (structure_changed || view_changed) {
		shown_project_revision = context.project.revision;
		shown_selection_revision = context.selection.revision;
		shown_viewport_revision = viewport.revision;
		clamp_vertical_scroll();
		layout_headers();
		ruler.repaint();
		timeline.repaint();
		return;
	}

	if (const auto position = context.engine.transport.position(); position != shown_position) {
		shown_position = position;
		timeline.repaint();
	}
}

void TrackAreaView::sync_headers()
{
	const auto& tracks = context.project.tracks;

	std::erase_if(headers, [&](const auto& entry) { return context.project.find_track(entry.first) == nullptr; });

	for (const auto& track : tracks) {
		auto& header = headers[track.id];

		if (header == nullptr) {
			header = std::make_unique<TrackHeader>(context, track.id);
			header_column.addAndMakeVisible(*header);
		}

		header->show(track, context.selection.contains(track.id));
	}
}

void TrackAreaView::layout_headers()
{
	const auto& tracks = context.project.tracks;

	for (size_t position = 0; position < tracks.size(); ++position)
		headers[tracks[position].id]->setBounds(0, (int) position * TimelineViewport::TRACK_HEIGHT - viewport.scroll_y, header_column.getWidth(), TimelineViewport::TRACK_HEIGHT);
}

void TrackAreaView::follow_playhead()
{
	if (!context.engine.transport.is_playing() || timeline.getWidth() <= 0)
		return;

	const auto x = viewport.beat_to_x(timeline.playhead_beat());

	if (x >= 0.0 && x <= timeline.getWidth() * FOLLOW_RIGHT_EDGE)
		return;

	viewport.first_beat = std::max(0.0, timeline.playhead_beat() - timeline.getWidth() * FOLLOW_LEFT_MARGIN / viewport.pixels_per_beat);
	++viewport.revision;
}

int TrackAreaView::content_height() const
{
	return (int) context.project.tracks.size() * TimelineViewport::TRACK_HEIGHT;
}

void TrackAreaView::clamp_vertical_scroll()
{
	viewport.scroll_y = std::clamp(viewport.scroll_y, 0, std::max(0, content_height() - timeline.getHeight()));
}

void TrackAreaView::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
	if (event.mods.isCommandDown()) {
		viewport.zoom_around(event.getEventRelativeTo(&timeline).position.x, std::exp(wheel.deltaY * WHEEL_ZOOM_SPEED));
		return;
	}

	const auto horizontal = event.mods.isShiftDown() || std::abs(wheel.deltaX) > std::abs(wheel.deltaY);

	if (horizontal) {
		const auto delta = std::abs(wheel.deltaX) > std::abs(wheel.deltaY) ? wheel.deltaX : wheel.deltaY;
		viewport.scroll_beats(-delta * WHEEL_SCROLL_PIXELS / viewport.pixels_per_beat);
		return;
	}

	viewport.scroll_y -= (int) (wheel.deltaY * WHEEL_SCROLL_PIXELS);
	++viewport.revision;
}

void TrackAreaView::paint(juce::Graphics& graphics)
{
	graphics.fillAll(theme::PANEL_BACKGROUND);
	graphics.setColour(theme::PANEL_BORDER);
	graphics.drawVerticalLine(TrackHeader::WIDTH, 0.0f, (float) getHeight());
}

void TrackAreaView::resized()
{
	auto area = getLocalBounds();
	auto top = area.removeFromTop(RulerView::HEIGHT);

	add_track_button.setBounds(top.removeFromLeft(TrackHeader::WIDTH).reduced(ADD_BUTTON_MARGIN));
	ruler.setBounds(top);
	header_column.setBounds(area.removeFromLeft(TrackHeader::WIDTH));
	timeline.setBounds(area.withTrimmedLeft(1));
	++viewport.revision;
}
