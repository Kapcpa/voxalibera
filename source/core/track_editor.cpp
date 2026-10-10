#include "core/track_editor.h"

#include "core/project/track_colours.h"

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

TrackId TrackEditor::add(const NewTrack& new_track)
{
	Track track;
	track.name = new_track.name.isNotEmpty() ? new_track.name : next_default_name();
	track.channels = new_track.channels;
	track.input = new_track.input;
	track.colour = TRACK_COLOURS[(size_t) project.next_id % TRACK_COLOURS.size()].argb;

	auto edit = edits.begin("Add Track");
	const auto id = edit.add_track(track, insert_position());
	selection.select_only(id);
	return id;
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
