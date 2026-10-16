#pragma once

#include "core/project/edit_history.h"
#include "core/selection.h"

struct TrackSettings
{
	juce::String name;
	TrackChannels channels = TrackChannels::mono;
	TrackInput input;
};

class TrackEditor
{
public:
	TrackEditor(Project& project, EditHistory& edits, Selection& selection);

	TrackId add(const TrackSettings& settings);
	void change_settings(TrackId id, const TrackSettings& settings);
	void remove_selected();
	void rename(TrackId id, const juce::String& name);
	void set_colour(TrackId id, std::uint32_t colour);
	void set_volume(TrackId id, float volume_db);
	void set_pan(TrackId id, float pan);
	void set_master_volume(float volume_db);
	void toggle(TrackId id, bool Track::* flag, const juce::String& action_name);
	void toggle_selected(bool Track::* flag, const juce::String& action_name);
	juce::String next_default_name() const;

	PluginId add_plugin(TrackId id, const Plugin& plugin);
	PluginId insert_plugin(TrackId id, const Plugin& plugin, size_t position);
	void remove_plugin(PluginId id);
	void move_plugin(PluginId id, TrackId target, size_t position);
	void toggle_plugin_bypass(PluginId id);

private:
	size_t insert_position() const;

	Project& project;
	EditHistory& edits;
	Selection& selection;
};

struct ClipMove
{
	std::int64_t delta = 0;
	int track_offset = 0;

	bool is_empty() const { return delta == 0 && track_offset == 0; }
};

class ClipEditor
{
public:
	ClipEditor(Project& project, EditHistory& edits, Selection& selection);

	ClipMove limit_move(ClipMove move) const;
	void move_selected(ClipMove move);
	void remove_selected();

private:
	Project& project;
	EditHistory& edits;
	Selection& selection;
};
