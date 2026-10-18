#include "core/editors.h"

#include "core/project/model.h"

#include <limits>

namespace
{
	constexpr auto DEFAULT_NAME = "Audio";
	constexpr float MIN_VOLUME_DB = -60.0f;
	constexpr float MAX_VOLUME_DB = 6.0f;
}

TrackEditor::TrackEditor(Project& project, EditHistory& edits, Selection& selection) : project(project), edits(edits), selection(selection)
{
}

juce::String TrackEditor::next_default_name() const
{
	return juce::String(DEFAULT_NAME) + " " + juce::String(project.tracks.size() + 1);
}

size_t TrackEditor::insert_position() const
{
	int position = Project::NOT_FOUND;

	for (const auto id : selection.tracks)
		position = std::max(position, project.index_of(id));

	return position == Project::NOT_FOUND ? project.tracks.size() : (size_t) position + 1;
}

TrackId TrackEditor::add(const TrackSettings& settings)
{
	auto edit = edits.begin("Add Track");
	return add(edit, settings);
}

TrackId TrackEditor::add(Edit& edit, const TrackSettings& settings)
{
	Track track;
	track.name = settings.name.isNotEmpty() ? settings.name : next_default_name();
	track.channels = settings.channels;
	track.input = settings.input;
	track.colour = TRACK_COLOURS[(size_t) project.next_id % TRACK_COLOURS.size()].argb;

	const auto id = edit.add_track(track, insert_position());
	selection.select_only(id);
	return id;
}

void TrackEditor::change_settings(TrackId id, const TrackSettings& settings)
{
	const auto* track = project.find_track(id);

	if (track == nullptr)
		return;

	const auto name = settings.name.trim().isNotEmpty() ? settings.name.trim() : track->name;

	if (name == track->name && settings.channels == track->channels && settings.input == track->input)
		return;

	auto edit = edits.begin("Track Settings");
	edit.set(id, &Track::name, name);
	edit.set(id, &Track::channels, settings.channels);
	edit.set(id, &Track::input, settings.input);
}

void TrackEditor::remove_selected()
{
	if (selection.tracks.empty())
		return;

	auto edit = edits.begin(selection.tracks.size() == 1 ? "Delete Track" : "Delete Tracks");

	for (const auto id : selection.tracks)
		edit.remove_track(id);

	selection.clear();
}

void TrackEditor::rename(TrackId id, const juce::String& name)
{
	if (name.trim().isEmpty())
		return;

	edits.begin("Rename Track").set(id, &Track::name, name.trim());
}

void TrackEditor::set_colour(TrackId id, std::uint32_t colour)
{
	edits.begin("Track Colour").set(id, &Track::colour, colour);
}

void TrackEditor::set_volume(TrackId id, float volume_db)
{
	edits.begin("Track Volume", "volume:" + juce::String(id.value)).set(id, &Track::volume_db, std::clamp(volume_db, MIN_VOLUME_DB, MAX_VOLUME_DB));
}

void TrackEditor::set_pan(TrackId id, float pan)
{
	edits.begin("Track Pan", "pan:" + juce::String(id.value)).set(id, &Track::pan, std::clamp(pan, -1.0f, 1.0f));
}

void TrackEditor::set_master_volume(float volume_db)
{
	edits.begin("Master Volume", "master-volume").set(&Project::master_volume_db, std::clamp(volume_db, MIN_VOLUME_DB, MAX_VOLUME_DB));
}

void TrackEditor::toggle(TrackId id, bool Track::* flag, const juce::String& action_name)
{
	if (const auto* track = project.find_track(id))
		edits.begin(action_name).set(id, flag, !(track->*flag));
}

void TrackEditor::toggle_selected(bool Track::* flag, const juce::String& action_name)
{
	if (selection.tracks.empty())
		return;

	const auto* first = project.find_track(selection.tracks.front());
	const auto value = first != nullptr && !(first->*flag);
	auto edit = edits.begin(action_name);

	for (const auto id : selection.tracks)
		edit.set(id, flag, value);
}

PluginId TrackEditor::add_plugin(TrackId id, const Plugin& plugin)
{
	const auto* track = project.find_track(id);
	return track == nullptr ? PluginId{} : insert_plugin(id, plugin, track->plugins.size());
}

PluginId TrackEditor::insert_plugin(TrackId id, const Plugin& plugin, size_t position)
{
	return edits.begin("Add " + plugin.name).add_plugin(id, plugin, position);
}

void TrackEditor::remove_plugin(PluginId id)
{
	const auto* plugin = project.find_plugin(id);

	if (plugin == nullptr)
		return;

	edits.begin("Remove " + plugin->name).remove_plugin(id);

	if (selection.plugin == id)
		selection.clear_chain();
}

void TrackEditor::move_plugin(PluginId id, TrackId target, size_t position)
{
	const auto* plugin = project.find_plugin(id);
	const auto* source = project.find_track_of(id);

	if (plugin == nullptr || project.find_track(target) == nullptr)
		return;

	const auto current_position = (size_t) (plugin - source->plugins.data());
	const auto last_position = source->id == target ? source->plugins.size() - 1 : project.find_track(target)->plugins.size();
	position = std::min(position, last_position);

	if (source->id == target && position == current_position)
		return;

	edits.begin("Move " + plugin->name).move_plugin(id, target, position);
}

void TrackEditor::toggle_plugin_bypass(PluginId id)
{
	if (const auto* plugin = project.find_plugin(id))
		edits.begin("Bypass " + plugin->name).set(id, &Plugin::bypass, !plugin->bypass);
}

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
