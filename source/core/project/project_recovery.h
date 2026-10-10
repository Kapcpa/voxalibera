#pragma once

#include "core/project/model.h"

#include <juce_core/juce_core.h>

#include <mutex>
#include <optional>
#include <vector>

struct UnsavedSession
{
	juce::File folder;
	int takes = 0;
	juce::int64 bytes = 0;
	juce::Time time;
	bool has_autosave = false;
};

juce::File autosave_file(const juce::File& project_folder);
bool crashed_in(const juce::File& project_folder);
std::optional<juce::Time> newer_autosave(const juce::File& project_file);
std::vector<UnsavedSession> leftover_unsaved_sessions();
Project project_from_takes(const juce::File& folder);

class ProjectRecovery : private juce::TimeSliceClient
{
public:
	explicit ProjectRecovery(juce::TimeSliceThread& disk_thread);
	~ProjectRecovery() override;

	void hold_lock(const juce::File& project_folder);
	void autosave(const juce::File& project_folder, juce::String text);
	void forget_autosave(const juce::File& project_folder);

private:
	struct PendingWrite
	{
		juce::File file;
		juce::String text;
	};

	int useTimeSlice() override;
	void release_lock();

	juce::TimeSliceThread& disk_thread;
	juce::File locked_folder;
	std::mutex mutex;
	std::optional<PendingWrite> pending;
};
