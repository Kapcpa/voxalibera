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
	constexpr float CLIP_MARGIN = 3.0f;
}

void TimelineViewport::zoom_around(double x, double factor)
{
	const auto anchor_beat = x_to_beat(x);
	pixels_per_beat = std::clamp(pixels_per_beat * factor, MIN_PIXELS_PER_BEAT, MAX_PIXELS_PER_BEAT);
	first_beat = std::max(0.0, anchor_beat - x / pixels_per_beat);
	++revision;
}

void TimelineViewport::scroll_beats(double beats)
{
	first_beat = std::max(0.0, first_beat + beats);
	++revision;
}

TimelineView::TimelineView(AppContext& context, const TimelineViewport& viewport) : context(context), viewport(viewport)
{
}

double TimelineView::sample_rate() const
{
	return timeline_rate(context.project, context.engine.sample_rate.load());
}

double TimelineView::playhead_beat() const
{
	const auto rate = context.engine.sample_rate.load();
	return rate > 0.0 ? (double) context.engine.transport.position() / samples_per_beat(context.project.tempo, rate) : 0.0;
}

double TimelineView::sample_to_x(std::int64_t sample) const
{
	const auto rate = sample_rate();
	return rate > 0.0 ? viewport.beat_to_x((double) sample / samples_per_beat(context.project.tempo, rate)) : 0.0;
}

juce::Rectangle<float> TimelineView::lane_area(size_t track_index) const
{
	return {0.0f, (float) ((int) track_index * TimelineViewport::TRACK_HEIGHT - viewport.scroll_y), (float) getWidth(), (float) TimelineViewport::TRACK_HEIGHT};
}

juce::Rectangle<float> TimelineView::clip_area(size_t track_index, std::int64_t start, std::int64_t length) const
{
	const auto lane = lane_area(track_index).reduced(0.0f, CLIP_MARGIN);
	const auto left = (float) sample_to_x(start);
	return lane.withX(left).withWidth((float) sample_to_x(start + length) - left);
}

void TimelineView::paint(juce::Graphics& graphics)
{
	graphics.fillAll(theme::WINDOW_BACKGROUND);
	paint_lanes(graphics);
	paint_grid(graphics);
	paint_clips(graphics);
	paint_live_takes(graphics);
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

void TimelineView::paint_clips(juce::Graphics& graphics)
{
	if (sample_rate() <= 0.0)
		return;

	const auto& tracks = context.project.tracks;

	for (size_t index = 0; index < tracks.size(); ++index)
		for (const auto& clip : tracks[index].clips) {
			const auto dragged = drag && context.selection.contains(clip.id);
			const auto start = dragged ? clip.start + drag->move.delta : clip.start;
			const auto lane = dragged ? (size_t) ((int) index + drag->move.track_offset) : index;
			const auto area = clip_area(lane, start, clip.length);

			if (area.getRight() < 0.0f || area.getX() > (float) getWidth())
				continue;

			paint_clip(graphics, area, {media_display_name(clip.file), juce::Colour(tracks[index].colour), context.peaks.find(clip.file), clip.source_offset, clip.length, sample_rate(), context.selection.contains(clip.id), false});
		}
}

void TimelineView::paint_live_takes(juce::Graphics& graphics)
{
	const auto start = context.recorder.start_sample();

	if (start == RecordingSession::NOT_STARTED || sample_rate() <= 0.0)
		return;

	const auto device_rate = context.engine.sample_rate.load();
	const auto timeline_start = convert_samples(start, device_rate, sample_rate());
	const auto length = convert_samples(context.engine.transport.position() - start, device_rate, sample_rate());

	for (const auto& take : context.recorder.live_takes())
		if (const auto index = context.project.index_of(take.track); index != Project::NOT_FOUND)
			paint_clip(graphics, clip_area((size_t) index, timeline_start, length), {take.file.getFileNameWithoutExtension(), theme::RECORD, &take.peaks, 0, length, sample_rate(), false, true});
}

std::optional<ClipId> TimelineView::clip_at(juce::Point<int> position) const
{
	const auto& tracks = context.project.tracks;

	for (size_t index = 0; index < tracks.size(); ++index)
		for (const auto& clip : tracks[index].clips)
			if (clip_area(index, clip.start, clip.length).contains(position.toFloat()))
				return clip.id;

	return std::nullopt;
}

int TimelineView::track_index_at(int y) const
{
	return (y + viewport.scroll_y) / TimelineViewport::TRACK_HEIGHT;
}

void TimelineView::mouseDown(const juce::MouseEvent& event)
{
	if (const auto clip = clip_at(event.getPosition()); clip && sample_rate() > 0.0) {
		if (event.mods.isShiftDown()) {
			context.selection.toggle(*clip);
			return;
		}

		if (!context.selection.contains(*clip))
			context.selection.select_only(*clip);

		drag = ClipDrag{*clip, context.project.find_clip(*clip)->start, viewport.x_to_beat(event.x), track_index_at(event.y), {}};
		return;
	}

	const auto index = track_index_at(event.y);
	const auto& tracks = context.project.tracks;

	if (index < 0 || index >= (int) tracks.size()) {
		context.selection.clear();
		return;
	}

	if (event.mods.isShiftDown())
		context.selection.toggle(tracks[(size_t) index].id);
	else
		context.selection.select_only(tracks[(size_t) index].id);
}

void TimelineView::mouseDrag(const juce::MouseEvent& event)
{
	if (!drag || !event.mouseWasDraggedSinceMouseDown())
		return;

	const auto spacing = samples_per_beat(context.project.tempo, sample_rate());
	const auto start_beat = (double) drag->anchor_start / spacing + viewport.x_to_beat(event.x) - drag->grab_beat;
	const auto snapped_beat = event.mods.isCommandDown() ? start_beat : std::round(start_beat);
	const auto new_start = (std::int64_t) (std::max(0.0, snapped_beat) * spacing);

	drag->move = context.clips.limit_move({new_start - drag->anchor_start, track_index_at(event.y) - drag->grab_track});
	repaint();
}

void TimelineView::mouseUp(const juce::MouseEvent& event)
{
	if (!drag)
		return;

	if (event.mouseWasDraggedSinceMouseDown())
		context.clips.move_selected(drag->move);
	else
		context.selection.select_only(drag->anchor);

	drag.reset();
	repaint();
}
