#include "ui/tracks/timeline_mouse.h"

#include <cmath>

namespace
{
	constexpr float HANDLE_SIZE = 8.0f;
	constexpr float GAIN_DB_PER_PIXEL = 0.2f;
}

TimelineMouse::TimelineMouse(AppContext& context, const TimelineLayout& layout) : context(context), layout(layout)
{
}

ClipZone TimelineMouse::zone_at(juce::Point<int> position, juce::Rectangle<float> clip_area) const
{
	const auto point = position.toFloat();

	if (point.y > clip_area.getY() + HANDLE_SIZE)
		return ClipZone::body;

	if (point.x < clip_area.getX() + HANDLE_SIZE)
		return ClipZone::fade_in;

	if (point.x > clip_area.getRight() - HANDLE_SIZE)
		return ClipZone::fade_out;

	return ClipZone::gain;
}

juce::MouseCursor TimelineMouse::cursor_at(juce::Point<int> position) const
{
	const auto clip = layout.clip_at(position);

	if (!clip)
		return juce::MouseCursor::NormalCursor;

	switch (zone_at(position, clip->area)) {
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
	current = ClipGesture{zone_at(event.getPosition(), clip->area), clip->id, model_clip->start, layout.viewport.x_to_beat(event.x), layout.track_index_at(event.y), model_clip->gain_db, {}};
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
