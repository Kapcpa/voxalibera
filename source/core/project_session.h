#pragma once

#include "core/device_session.h"
#include "core/media/peaks.h"
#include "core/project/edit_history.h"
#include "core/selection.h"
#include "engine/audio_engine.h"

#include <juce_core/juce_core.h>

struct RecentProjects
{
	static constexpr int MAX_FILES = 10;

	static RecentProjects load();
	void add(const juce::File& file);
	void remove(const juce::File& file);
	void save() const;

	juce::Array<juce::File> files;
};

class ProjectSession
{
public:
	static constexpr int MAX_BACKUPS = 10;

	ProjectSession(AudioEngine& engine, DeviceSession& devices, Project& project, EditHistory& edits, Selection& selection, PeakStore& peaks, PeakBuilder& peak_builder);

	static juce::File projects_folder();

	bool is_saved() const { return file != juce::File(); }
	bool is_read_only() const { return read_only; }
	bool has_unsaved_changes() const { return project.revision != saved_revision; }
	juce::String name() const;
	const juce::File& project_file() const { return file; }

	void start_new();
	juce::Result open(const juce::File& chosen, juce::StringArray& problems);
	juce::Result save();
	juce::Result save_as(const juce::File& chosen);
	void discard_unsaved_audio();

	RecentProjects recent = RecentProjects::load();

private:
	void replace_project(Project next);
	juce::Result move_media_to(const juce::File& folder);
	juce::Result write_to(const juce::File& target);
	void back_up(const juce::File& target);
	void request_missing_peaks();

	AudioEngine& engine;
	DeviceSession& devices;
	Project& project;
	EditHistory& edits;
	Selection& selection;
	PeakStore& peaks;
	PeakBuilder& peak_builder;
	juce::File file;
	bool read_only = false;
	int saved_revision = 0;
};
