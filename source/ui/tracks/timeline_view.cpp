#include "ui/tracks/timeline_view.h"

#include "core/media/project_folders.h"
#include "ui/theme.h"
#include "ui/tracks/clip_painter.h"

#include <algorithm>
#include <cmath>

namespace
{
	constexpr double MIN_BEAT_LINE_SPACING = 6.0;
	constexpr float PLAYHEAD_WIDTH = 1.5f;
	constexpr float HINT_FONT_SIZE = 14.0f;
	constexpr auto EMPTY_HINT = "No tracks yet. Press Ctrl+T or + Track to add one.";
	constexpr float DROP_LINE_WIDTH = 2.0f;
}

TimelineView::TimelineView(AppContext& context, const TimelineViewport& viewport) : context(context), viewport(viewport), layout{context, viewport}, mouse(context, layout)
{
}

double TimelineView::playhead_beat() const
{
	const auto rate = context.engine.sample_rate.load();
	return rate > 0.0 ? (double) context.engine.transport.position() / samples_per_beat(context.project.tempo, rate) : 0.0;
}

bool TimelineView::isInterestedInFileDrag(const juce::StringArray& files)
{
	if (context.recorder.is_recording())
		return false;

	return std::any_of(files.begin(), files.end(), [](const auto& path) { return is_importable_audio(juce::File(path)); });
}

ImportPlace TimelineView::import_place_at(int x, int y) const
{
	const auto index = layout.track_index_at(y);
	const auto& tracks = context.project.tracks;
	const auto spacing = samples_per_beat(context.project.tempo, layout.sample_rate());
	const auto beat = std::max(0.0, viewport.x_to_beat(x));
	const auto snapped_beat = juce::ModifierKeys::currentModifiers.isCommandDown() ? beat : std::round(beat);

	ImportPlace place;
	place.start = spacing > 0.0 ? (std::int64_t) (snapped_beat * spacing) : 0;

	if (index >= 0 && index < (int) tracks.size())
		place.track = tracks[(size_t) index].id;

	return place;
}

void TimelineView::fileDragMove(const juce::StringArray&, int x, int y)
{
	drop_place = import_place_at(x, y);
	repaint();
}

void TimelineView::fileDragExit(const juce::StringArray&)
{
	drop_place.reset();
	repaint();
}

void TimelineView::filesDropped(const juce::StringArray& files, int x, int y)
{
	const auto place = import_place_at(x, y);
	drop_place.reset();
	repaint();

	for (const auto& path : files)
		import_audio_file(context, juce::File(path), place);
}

void TimelineView::paint(juce::Graphics& graphics)
{
	graphics.fillAll(theme::WINDOW_BACKGROUND);
	paint_lanes(graphics);
	paint_grid(graphics);
	paint_clips(graphics);
	paint_live_takes(graphics);
	paint_drop_place(graphics);
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

void TimelineView::paint_drop_place(juce::Graphics& graphics)
{
	if (!drop_place)
		return;

	const auto row = drop_place->track ? context.project.index_of(*drop_place->track) : (int) context.project.tracks.size();
	const auto y = row * TimelineViewport::TRACK_HEIGHT - viewport.scroll_y;

	graphics.setColour(theme::DROP_TARGET);
	graphics.fillRect(0, y, getWidth(), TimelineViewport::TRACK_HEIGHT);

	graphics.setColour(theme::ACCENT);
	graphics.fillRect((float) layout.sample_to_x(drop_place->start), (float) y, DROP_LINE_WIDTH, (float) TimelineViewport::TRACK_HEIGHT);
}

void TimelineView::paint_playhead(juce::Graphics& graphics)
{
	const auto x = (float) viewport.beat_to_x(playhead_beat());

	if (x < 0.0f || x > (float) getWidth())
		return;

	graphics.setColour(theme::ACCENT);
	graphics.fillRect(x, 0.0f, PLAYHEAD_WIDTH, (float) getHeight());
}

void TimelineView::paint_clips(juce::Graphics& graphics)
{
	if (layout.sample_rate() <= 0.0)
		return;

	const auto& tracks = context.project.tracks;

	for (size_t index = 0; index < tracks.size(); ++index)
		for (const auto& clip : tracks[index].clips) {
			const auto& gesture = mouse.gesture();
			const auto moved = gesture && gesture->zone == ClipZone::body && context.selection.contains(clip.id);
			const auto start = moved ? clip.start + gesture->move.delta : clip.start;
			const auto lane = moved ? (size_t) ((int) index + gesture->move.track_offset) : index;
			const auto area = layout.clip_area(lane, start, clip.length);

			if (area.getRight() < 0.0f || area.getX() > (float) getWidth())
				continue;

			paint_clip(graphics, area, {media_display_name(clip.file), juce::Colour(tracks[index].colour), context.peaks.find(clip.file), clip.source_offset, clip.length, layout.sample_rate(), channel_count(tracks[index].channels), context.selection.contains(clip.id), false, clip.gain_db});
		}
}

void TimelineView::paint_live_takes(juce::Graphics& graphics)
{
	const auto start = context.recorder.start_sample();

	if (start == RecordingSession::NOT_STARTED || layout.sample_rate() <= 0.0)
		return;

	const auto device_rate = context.engine.sample_rate.load();
	const auto timeline_start = convert_samples(start, device_rate, layout.sample_rate());
	const auto length = convert_samples(context.engine.transport.position() - start, device_rate, layout.sample_rate());

	for (const auto& take : context.recorder.live_takes())
		if (const auto index = context.project.index_of(take.track); index != Project::NOT_FOUND)
			paint_clip(graphics, layout.clip_area((size_t) index, timeline_start, length), {take.file.getFileNameWithoutExtension(), theme::RECORD, &take.peaks, 0, length, layout.sample_rate(), channel_count(context.project.tracks[(size_t) index].channels), false, true});
}

void TimelineView::mouseDown(const juce::MouseEvent& event)
{
	mouse.down(event);
	repaint();
}

void TimelineView::mouseDrag(const juce::MouseEvent& event)
{
	mouse.drag(event);
	repaint();
}

void TimelineView::mouseUp(const juce::MouseEvent& event)
{
	mouse.up(event);
	setMouseCursor(mouse.cursor_at(event.getPosition()));
	repaint();
}

void TimelineView::mouseMove(const juce::MouseEvent& event)
{
	setMouseCursor(mouse.cursor_at(event.getPosition()));
}
