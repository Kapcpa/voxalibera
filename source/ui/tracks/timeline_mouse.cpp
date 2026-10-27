#include "ui/tracks/timeline_mouse.h"

#include "ui/tracks/clip_painter.h"

#include <cmath>

namespace
{
	constexpr float HANDLE_SIZE = 8.0f;
	constexpr float GAIN_DB_PER_PIXEL = 0.2f;
}

TimelineMouse::TimelineMouse(AppContext& context, const TimelineLayout& layout) : context(context), layout(layout)
{
}

ClipZone TimelineMouse::zone_at(juce::Point<int> position, const ClipUnder& clip) const
{
	const auto point = position.toFloat();
	const auto* model_clip = context.project.find_clip(clip.id);
	const auto handles = fade_handles(clip.area, model_clip->length, model_clip->fade_in, model_clip->fade_out);
	const auto to_fade_in = point.getDistanceFrom({handles.fade_in_x, handles.y});
	const auto to_fade_out = point.getDistanceFrom({handles.fade_out_x, handles.y});

	if (std::min(to_fade_in, to_fade_out) < HANDLE_SIZE)
		return to_fade_in <= to_fade_out ? ClipZone::fade_in : ClipZone::fade_out;

	if (point.y < clip.area.getY() + CLIP_HEADER_HEIGHT)
		return ClipZone::gain;

	return ClipZone::body;
}

juce::MouseCursor TimelineMouse::cursor_at(juce::Point<int> position) const
{
	const auto clip = layout.clip_at(position);

	if (!clip)
		return juce::MouseCursor::NormalCursor;

	switch (zone_at(position, *clip)) {
		case ClipZone::fade_in:
		case ClipZone::fade_out:
			return juce::MouseCursor::LeftRightResizeCursor;
		case ClipZone::gain:
			return juce::MouseCursor::UpDownResizeCursor;
		case ClipZone::body:
			break;
	}

	return juce::MouseCursor::NormalCursor;
}

void TimelineMouse::down(const juce::MouseEvent& event)
{
	const auto clip = layout.clip_at(event.getPosition());

	if (!clip || layout.sample_rate() <= 0.0) {
		select_track_at(event);
		return;
	}

	if (event.mods.isShiftDown()) {
		context.selection.toggle(clip->id);
		return;
	}

	if (!context.selection.contains(clip->id))
		context.selection.select_only(clip->id);

	const auto* model_clip = context.project.find_clip(clip->id);
	current = ClipGesture{zone_at(event.getPosition(), *clip), clip->id, model_clip->start, layout.viewport.x_to_beat(event.x), layout.track_index_at(event.y), model_clip->gain_db, {}};
}

void TimelineMouse::select_track_at(const juce::MouseEvent& event)
{
	const auto index = layout.track_index_at(event.y);
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

void TimelineMouse::drag(const juce::MouseEvent& event)
{
	if (!current || !event.mouseWasDraggedSinceMouseDown())
		return;

	if (current->zone == ClipZone::body)
		drag_move(event);
	else if (current->zone == ClipZone::gain)
		drag_gain(event);
	else if (current->zone == ClipZone::fade_in)
		drag_fade_in(event);
	else if (current->zone == ClipZone::fade_out)
		drag_fade_out(event);
}

void TimelineMouse::drag_move(const juce::MouseEvent& event)
{
	const auto spacing = samples_per_beat(context.project.tempo, layout.sample_rate());
	const auto start_beat = (double) current->anchor_start / spacing + layout.viewport.x_to_beat(event.x) - current->grab_beat;
	const auto snapped_beat = event.mods.isCommandDown() ? start_beat : std::round(start_beat);
	const auto new_start = (std::int64_t) (std::max(0.0, snapped_beat) * spacing);

	current->move = context.clips.limit_move({new_start - current->anchor_start, layout.track_index_at(event.y) - current->grab_track});
}

void TimelineMouse::drag_gain(const juce::MouseEvent& event)
{
	const auto new_gain_db = current->gain_db - event.getDistanceFromDragStartY() * GAIN_DB_PER_PIXEL;

	context.clips.set_gain(current->anchor, new_gain_db);
}

void TimelineMouse::drag_fade_in(const juce::MouseEvent& event)
{
	const auto* model_clip = context.project.find_clip(current->anchor);
	const auto new_fade_in = layout.viewport.x_to_beat(event.x) * samples_per_beat(context.project.tempo, layout.sample_rate()) - model_clip->start;

	context.clips.set_fade_in(current->anchor, (std::int64_t) new_fade_in);
}

void TimelineMouse::drag_fade_out(const juce::MouseEvent& event)
{
	const auto* model_clip = context.project.find_clip(current->anchor);
	const auto new_fade_out = model_clip->start + model_clip->length - layout.viewport.x_to_beat(event.x) * samples_per_beat(context.project.tempo, layout.sample_rate());

	context.clips.set_fade_out(current->anchor, (std::int64_t) new_fade_out);
}

void TimelineMouse::up(const juce::MouseEvent& event)
{
	if (!current)
		return;

	if (!event.mouseWasDraggedSinceMouseDown())
		context.selection.select_only(current->anchor);
	else if (current->zone == ClipZone::body)
		context.clips.move_selected(current->move);

	current.reset();
}
