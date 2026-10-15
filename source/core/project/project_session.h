#pragma once

#include "core/audio/device_session.h"
#include "core/plugins/plugin_pool.h"
#include "core/media/peaks.h"
#include "core/project/edit_history.h"
#include "core/project/project_recovery.h"
#include "core/project/recent_projects.h"
#include "core/selection.h"
#include "engine/audio_engine.h"

#include <juce_core/juce_core.h>

class ProjectSession
{
public:
	static constexpr int MAX_BACKUPS = 10;
	static constexpr double AUTOSAVE_SECONDS = 300.0;

	ProjectSession(AudioEngine& engine, DeviceSession& devices, Project& project, EditHistory& edits, Selection& selection, PeakStore& peaks, PeakBuilder& peak_builder, PluginPool& plugins, juce::TimeSliceThread& disk_thread);

	static juce::File projects_folder();

	bool is_saved() const { return file != juce::File(); }
	bool is_read_only() const { return read_only; }
	bool has_unsaved_changes() const { return project.revision != saved_revision; }
	juce::String name() const;
	const juce::File& project_file() const { return file; }

	void update(double now);
	void start_new();
	juce::Result open(const juce::File& chosen, juce::StringArray& problems, bool restore_autosave = false);
	void recover(const UnsavedSession& unsaved);
	juce::Result save();
	juce::Result save_as(const juce::File& chosen);
	void discard_changes();
	int clean_up(const juce::Array<juce::File>& unused);

	RecentProjects recent = RecentProjects::load();

private:
	void replace_project(Project next);
	juce::Result move_media_to(const juce::File& folder);
	juce::Result write_to(const juce::File& target);
	void back_up(const juce::File& target);
	void request_missing_peaks();
	void autosave_if_due(double now);

	AudioEngine& engine;
	DeviceSession& devices;
	Project& project;
	EditHistory& edits;
	Selection& selection;
	PeakStore& peaks;
	PeakBuilder& peak_builder;
	PluginPool& plugins;
	ProjectRecovery recovery;
	juce::File file;
	bool read_only = false;
	int saved_revision = 0;
	int autosaved_revision = 0;
	std::optional<double> autosave_due;
};
