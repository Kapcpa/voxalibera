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
	constexpr float REORDER_LINE_HEIGHT = 2.0f;
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

	const auto position = context.engine.transport.position();

	if (position != shown_position || context.peaks.revision != shown_peaks_revision) {
		shown_position = position;
		shown_peaks_revision = context.peaks.revision;
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
			header->on_show_plugins = [this](TrackId id) {
				if (on_show_plugins)
					on_show_plugins(id);
			};
			header->on_edit = [this](TrackId id) {
				if (on_edit_track)
					on_edit_track(id);
			};
			header->on_reorder_drag = [this](int y) { show_reorder_gap(y); };
			header->on_reorder_drop = [this](TrackId id, int y) { drop_track(id, y); };
			header_column.addAndMakeVisible(*header);
		}

		header->show(track, context.selection.contains(track.id));
	}
}

void TrackAreaView::layout_headers()
{
	const auto& tracks = context.project.tracks;

	const TimelineLayout layout{context, viewport};

	for (size_t position = 0; position < tracks.size(); ++position)
		headers[tracks[position].id]->setBounds(0, layout.track_top(position), header_column.getWidth(), track_height(tracks[position]));
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
	return tracks_height(context.project);
}

size_t TrackAreaView::gap_at(int y) const
{
	const TimelineLayout layout{context, viewport};
	const auto& tracks = context.project.tracks;
	const auto index = layout.track_index_at(y);

	if (index < 0)
		return 0;

	if ((size_t) index >= tracks.size())
		return tracks.size();

	const auto below_middle = y - layout.track_top((size_t) index) > track_height(tracks[(size_t) index]) / 2;
	return (size_t) index + (below_middle ? 1 : 0);
}

void TrackAreaView::show_reorder_gap(int y)
{
	reorder_gap = context.recorder.is_recording() ? std::nullopt : std::optional<size_t>(gap_at(y));
	repaint();
}

void TrackAreaView::drop_track(TrackId id, int y)
{
	reorder_gap.reset();
	repaint();

	const auto from = context.project.index_of(id);

	if (from == Project::NOT_FOUND || context.recorder.is_recording())
		return;

	const auto gap = gap_at(y);
	const auto position = gap > (size_t) from ? gap - 1 : gap;

	if (position != (size_t) from)
		context.tracks.move(id, position);
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

void TrackAreaView::paintOverChildren(juce::Graphics& graphics)
{
	if (!reorder_gap)
		return;

	const TimelineLayout layout{context, viewport};
	const auto y = (float) (header_column.getY() + layout.track_top(*reorder_gap)) - REORDER_LINE_HEIGHT / 2.0f;

	graphics.setColour(theme::ACCENT);
	graphics.fillRect(0.0f, y, (float) getWidth(), REORDER_LINE_HEIGHT);
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
