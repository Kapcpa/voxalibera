#include "core/clip_editor.h"

#include <limits>

ClipEditor::ClipEditor(Project& project, EditHistory& edits, Selection& selection) : project(project), edits(edits), selection(selection)
{
}

ClipMove ClipEditor::limit_move(ClipMove move) const
{
	auto earliest_start = std::numeric_limits<std::int64_t>::max();
	auto lowest_track = std::numeric_limits<int>::max();
	auto highest_track = 0;

	for (const auto id : selection.clips) {
		const auto* clip = project.find_clip(id);
		const auto* track = project.find_track_of(id);

		if (clip == nullptr || track == nullptr)
			continue;

		const auto index = project.index_of(track->id);
		earliest_start = std::min(earliest_start, clip->start);
		lowest_track = std::min(lowest_track, index);
		highest_track = std::max(highest_track, index);
	}

	if (lowest_track == std::numeric_limits<int>::max())
		return {};

	move.delta = std::max(move.delta, -earliest_start);
	move.track_offset = std::clamp(move.track_offset, -lowest_track, (int) project.tracks.size() - 1 - highest_track);
	return move;
}

void ClipEditor::move_selected(ClipMove move)
{
	move = limit_move(move);

	if (move.is_empty())
		return;

	auto edit = edits.begin(selection.clips.size() == 1 ? "Move Clip" : "Move Clips");

	for (const auto id : selection.clips) {
		const auto* clip = project.find_clip(id);
		const auto track_index = project.index_of(project.find_track_of(id)->id);
		edit.set(id, &Clip::start, clip->start + move.delta);
		edit.move_clip(id, project.tracks[(size_t) (track_index + move.track_offset)].id);
	}
}

void ClipEditor::remove_selected()
{
	if (selection.clips.empty())
		return;

	auto edit = edits.begin(selection.clips.size() == 1 ? "Delete Clip" : "Delete Clips");

	for (const auto id : selection.clips)
		edit.remove_clip(id);

	edit.commit();
	selection.forget_missing(project);
}
