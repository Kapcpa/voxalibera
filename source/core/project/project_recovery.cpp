#include "core/project/project_recovery.h"

#include "core/log.h"
#include "core/media/project_folders.h"
#include "core/project/project_cleanup.h"
#include "engine/audio_file.h"
#include "platform/system.h"

namespace
{
	constexpr auto LOCK_FILE = ".lock";
	constexpr auto AUTOSAVE_FOLDER = "autosave";
	constexpr auto AUTOSAVE_FILE = "autosave.vxl";
	constexpr int IDLE_MILLISECONDS = 500;

	juce::File lock_file(const juce::File& project_folder)
	{
		return project_folder.getChildFile(LOCK_FILE);
	}

	bool is_locked_by_running_app(const juce::File& project_folder)
	{
		return is_process_running(lock_file(project_folder).loadFileAsString().trim().getIntValue(), APPLICATION_NAME);
	}
}

juce::File autosave_file(const juce::File& project_folder)
{
	return project_folder.getChildFile(AUTOSAVE_FOLDER).getChildFile(AUTOSAVE_FILE);
}

bool crashed_in(const juce::File& project_folder)
{
	return lock_file(project_folder).existsAsFile() && !is_locked_by_running_app(project_folder);
}

std::optional<juce::Time> newer_autosave(const juce::File& project_file)
{
	const auto autosave = autosave_file(project_file.getParentDirectory());

	if (!autosave.existsAsFile() || autosave.getLastModificationTime() <= project_file.getLastModificationTime())
		return std::nullopt;

	return autosave.getLastModificationTime();
}

std::vector<UnsavedSession> leftover_unsaved_sessions()
{
	std::vector<UnsavedSession> sessions;

	for (const auto& folder : unsaved_project_folders()) {
		if (is_locked_by_running_app(folder))
			continue;

		const auto takes = audio_files_in(folder);
		const auto has_autosave = autosave_file(folder).existsAsFile();

		if (takes.isEmpty() && !has_autosave)
			continue;

		sessions.push_back({folder, takes.size(), total_size(takes), folder.getCreationTime(), has_autosave});
	}

	return sessions;
}

Project project_from_takes(const juce::File& folder)
{
	Project project;
	project.folder = folder;

	for (const auto& take : audio_files_in(folder)) {
		const auto file = AudioFile::open(take);

		if (file == nullptr)
			continue;

		Track track;
		track.id = project.allocate_track_id();
		track.name = take.getFileNameWithoutExtension();
		track.colour = TRACK_COLOURS[project.tracks.size() % TRACK_COLOURS.size()].argb;
		track.channels = file->channels() > 1 ? TrackChannels::stereo : TrackChannels::mono;
		track.input = {TrackInput::NONE};

		Clip clip;
		clip.id = project.allocate_clip_id();
		clip.file = relative_media_path(folder, take);
		if (project.sample_rate <= 0.0)
			project.sample_rate = file->sample_rate();

		clip.length = convert_samples(file->length(), file->sample_rate(), project.sample_rate);
		track.clips.push_back(clip);

		project.tracks.push_back(track);
	}

	return project;
}

ProjectRecovery::ProjectRecovery(juce::TimeSliceThread& disk_thread) : disk_thread(disk_thread)
{
	disk_thread.addTimeSliceClient(this);
}

ProjectRecovery::~ProjectRecovery()
{
	disk_thread.removeTimeSliceClient(this);
	useTimeSlice();
	release_lock();
}

void ProjectRecovery::hold_lock(const juce::File& project_folder)
{
	if (project_folder == locked_folder && (project_folder == juce::File() || lock_file(project_folder).existsAsFile()))
		return;

	release_lock();
	locked_folder = project_folder;

	if (project_folder.isDirectory())
		lock_file(project_folder).replaceWithText(juce::String(current_process_id()));
}

void ProjectRecovery::release_lock()
{
	if (locked_folder != juce::File() && lock_file(locked_folder).loadFileAsString().trim().getIntValue() == current_process_id())
		lock_file(locked_folder).deleteFile();

	locked_folder = juce::File();
}

void ProjectRecovery::autosave(const juce::File& project_folder, juce::String text)
{
	const std::scoped_lock lock(mutex);
	pending = PendingWrite{autosave_file(project_folder), std::move(text)};
}

void ProjectRecovery::forget_autosave(const juce::File& project_folder)
{
	const std::scoped_lock lock(mutex);
	pending.reset();
	autosave_file(project_folder).getParentDirectory().deleteRecursively();
}

int ProjectRecovery::useTimeSlice()
{
	const std::scoped_lock lock(mutex);

	if (!pending)
		return IDLE_MILLISECONDS;

	pending->file.getParentDirectory().createDirectory();

	if (!pending->file.replaceWithText(pending->text))
		log_event("Could not autosave to " + pending->file.getFullPathName());

	pending.reset();
	return 0;
}
