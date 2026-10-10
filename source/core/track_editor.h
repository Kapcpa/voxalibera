#pragma once

#include "core/project/edit_history.h"
#include "core/selection.h"

struct NewTrack
{
	juce::String name;
	TrackChannels channels = TrackChannels::mono;
	TrackInput input;
};

class TrackEditor
{
public:
	TrackEditor(Project& project, EditHistory& edits, Selection& selection);

	TrackId add(const NewTrack& new_track);
	void remove_selected();
	void rename(TrackId id, const juce::String& name);
	void set_colour(TrackId id, std::uint32_t colour);
	void set_volume(TrackId id, float volume_db);
	void toggle(TrackId id, bool Track::* flag, const juce::String& action_name);
	void toggle_selected(bool Track::* flag, const juce::String& action_name);
	juce::String next_default_name() const;

private:
	size_t insert_position() const;

	Project& project;
	EditHistory& edits;
	Selection& selection;
};
