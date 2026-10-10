#include "core/project/project_session.h"

#include "core/log.h"
#include "core/media/project_folders.h"
#include "core/project/project_cleanup.h"
#include "core/project/project_file.h"

namespace
{
	constexpr auto PROJECTS_FOLDER = "Voxalibera";
	constexpr auto BACKUP_FOLDER = "backups";
	constexpr auto BACKUP_TIME_FORMAT = "%Y-%m-%d_%H-%M-%S";
	constexpr auto UNTITLED = "Untitled";

	juce::File folder_for(const juce::File& chosen)
	{
		const auto name = chosen.getFileNameWithoutExtension();
		const auto parent = chosen.getParentDirectory();
		return parent.getFileName() == name ? parent : parent.getChildFile(name);
	}

	juce::Result move_folder(const juce::File& source, const juce::File& target)
	{
		if (source.moveFileTo(target))
			return juce::Result::ok();

		if (!source.copyDirectoryTo(target))
			return juce::Result::fail("Could not copy " + source.getFullPathName() + " to " + target.getFullPathName());

		source.deleteRecursively();
		return juce::Result::ok();
	}

	int count_missing_files(const Project& project)
	{
		int missing = 0;

		for (const auto& track : project.tracks)
			for (const auto& clip : track.clips)
				if (!project.folder.getChildFile(clip.file).existsAsFile())
					++missing;

		return missing;
	}
}

ProjectSession::ProjectSession(AudioEngine& engine, DeviceSession& devices, Project& project, EditHistory& edits, Selection& selection, PeakStore& peaks, PeakBuilder& peak_builder, juce::TimeSliceThread& disk_thread) : engine(engine), devices(devices), project(project), edits(edits), selection(selection), peaks(peaks), peak_builder(peak_builder), recovery(disk_thread)
{
}

juce::File ProjectSession::projects_folder()
{
	return juce::File::getSpecialLocation(juce::File::userMusicDirectory).getChildFile(PROJECTS_FOLDER);
}

juce::String ProjectSession::name() const
{
	return is_saved() ? file.getFileNameWithoutExtension() : juce::String(UNTITLED);
}

void ProjectSession::start_new()
{
	replace_project({});
	file = juce::File();
	read_only = false;
	devices.use_project_rate(0.0);
	log_event("New project");
}

void ProjectSession::update(double now)
{
	recovery.hold_lock(project.folder);
	autosave_if_due(now);
}

void ProjectSession::autosave_if_due(double now)
{
	if (!has_unsaved_changes() || project.revision == autosaved_revision || read_only) {
		autosave_due.reset();
		return;
	}

	if (!autosave_due)
		autosave_due = now + AUTOSAVE_SECONDS;

	if (now < *autosave_due)
		return;

	if (project.folder == juce::File())
		project.folder = new_unsaved_project_folder();

	recovery.autosave(project.folder, write_project(project, JUCE_APPLICATION_VERSION_STRING));
	autosaved_revision = project.revision;
	autosave_due.reset();
}

juce::Result ProjectSession::open(const juce::File& chosen, juce::StringArray& problems, bool restore_autosave)
{
	if (!chosen.existsAsFile()) {
		recent.remove(chosen);
		return juce::Result::fail(chosen.getFullPathName() + " can't be found.");
	}

	const auto source = restore_autosave ? autosave_file(chosen.getParentDirectory()) : chosen;
	auto loaded = read_project(source.loadFileAsString());

	if (!loaded) {
		recent.remove(chosen);
		return juce::Result::fail(chosen.getFileName() + " is not a Voxalibera project, or it is damaged.");
	}

	auto& next = loaded->project;
	next.folder = chosen.getParentDirectory();

	for (auto& track : next.tracks)
		track.armed = false;

	problems = loaded->problems;

	if (const auto missing = count_missing_files(next); missing > 0)
		problems.add(juce::String(missing) + " audio file(s) are missing from " + next.folder.getChildFile(AUDIO_FOLDER).getFullPathName());

	if (loaded->newer_format)
		problems.add("This project was saved by a newer Voxalibera. It opens read-only: use Save As to keep your changes in a copy.");

	replace_project(std::move(next));
	file = chosen;
	read_only = loaded->newer_format;

	if (restore_autosave)
		--saved_revision;

	recent.add(chosen);
	devices.use_project_rate(project.sample_rate);
	request_missing_peaks();
	log_event((restore_autosave ? "Restored the autosave of " : "Opened ") + chosen.getFullPathName() + (problems.isEmpty() ? juce::String() : ": " + problems.joinIntoString("; ")));
	return juce::Result::ok();
}

void ProjectSession::recover(const UnsavedSession& unsaved)
{
	const auto autosaved = unsaved.has_autosave ? read_project(autosave_file(unsaved.folder).loadFileAsString()) : std::nullopt;
	auto next = autosaved ? autosaved->project : project_from_takes(unsaved.folder);
	next.folder = unsaved.folder;

	for (auto& track : next.tracks)
		track.armed = false;

	replace_project(std::move(next));
	file = juce::File();
	read_only = false;
	--saved_revision;
	devices.use_project_rate(project.sample_rate);
	request_missing_peaks();
	log_event("Recovered the unsaved session in " + unsaved.folder.getFullPathName() + (autosaved ? " from its autosave" : " from its takes"));
}

void ProjectSession::replace_project(Project next)
{
	engine.transport.stop();
	engine.transport.locate(0);
	next.revision = project.revision + 1;
	project = std::move(next);
	saved_revision = project.revision;
	autosaved_revision = project.revision;
	autosave_due.reset();
	edits.clear();
	selection.clear();
	peak_builder.cancel_all();
	peaks.clear();
}

void ProjectSession::request_missing_peaks()
{
	for (const auto& track : project.tracks)
		for (const auto& clip : track.clips)
			if (peaks.find(clip.file) == nullptr)
				peak_builder.request(project.folder, clip.file);
}

juce::Result ProjectSession::save()
{
	if (!is_saved() || read_only)
		return juce::Result::fail("This project needs Save As.");

	return write_to(file);
}

juce::Result ProjectSession::save_as(const juce::File& chosen)
{
	const auto folder = folder_for(chosen);
	const auto target = folder.getChildFile(folder.getFileName() + PROJECT_EXTENSION);

	if (folder != project.folder && folder.exists() && !folder.isDirectory())
		return juce::Result::fail(folder.getFullPathName() + " is a file, not a folder.");

	if (folder != project.folder && folder.getNumberOfChildFiles(juce::File::findFilesAndDirectories) > 0)
		return juce::Result::fail("The folder " + folder.getFullPathName() + " already has files in it. Choose another name.");

	if (const auto created = folder.createDirectory(); created.failed())
		return created;

	if (const auto moved = move_media_to(folder); moved.failed())
		return moved;

	const auto written = write_to(target);

	if (written.failed())
		return written;

	file = target;
	read_only = false;
	recent.add(target);
	return written;
}

juce::Result ProjectSession::move_media_to(const juce::File& folder)
{
	const auto previous = project.folder;

	if (previous == juce::File() || previous == folder) {
		project.folder = folder;
		return juce::Result::ok();
	}

	const auto source = previous.getChildFile(AUDIO_FOLDER);
	const auto destination = folder.getChildFile(AUDIO_FOLDER);
	const auto unsaved = is_unsaved_project_folder(previous);

	if (source.isDirectory()) {
		const auto result = unsaved ? move_folder(source, destination) : juce::Result(source.copyDirectoryTo(destination) ? juce::Result::ok() : juce::Result::fail("Could not copy the audio files to " + destination.getFullPathName()));

		if (result.failed())
			return result;
	}

	if (unsaved)
		previous.deleteRecursively();

	project.folder = folder;
	++project.revision;
	log_event((unsaved ? "Moved recordings from " : "Copied recordings from ") + previous.getFullPathName() + " to " + folder.getFullPathName());
	return juce::Result::ok();
}

juce::Result ProjectSession::write_to(const juce::File& target)
{
	if (project.sample_rate <= 0.0 && engine.running.load())
		project.sample_rate = engine.sample_rate.load();

	back_up(target);

	if (!target.replaceWithText(write_project(project, JUCE_APPLICATION_VERSION_STRING)))
		return juce::Result::fail("Could not write " + target.getFullPathName());

	saved_revision = project.revision;
	autosaved_revision = project.revision;
	autosave_due.reset();
	recovery.forget_autosave(target.getParentDirectory());
	log_event("Saved " + target.getFullPathName());
	return juce::Result::ok();
}

void ProjectSession::back_up(const juce::File& target)
{
	if (!target.existsAsFile())
		return;

	const auto folder = target.getParentDirectory().getChildFile(BACKUP_FOLDER);
	folder.createDirectory();
	target.copyFileTo(folder.getChildFile(target.getFileNameWithoutExtension() + "_" + juce::Time::getCurrentTime().formatted(BACKUP_TIME_FORMAT) + PROJECT_EXTENSION));

	auto backups = folder.findChildFiles(juce::File::findFiles, false, juce::String("*") + PROJECT_EXTENSION);
	backups.sort();

	for (int index = 0; index < backups.size() - MAX_BACKUPS; ++index)
		backups[index].deleteFile();
}

void ProjectSession::discard_changes()
{
	if (is_saved() || !is_unsaved_project_folder(project.folder)) {
		if (project.folder != juce::File())
			recovery.forget_autosave(project.folder);

		return;
	}

	log_event("Discarded unsaved recordings in " + project.folder.getFullPathName());
	project.folder.deleteRecursively();
	project.folder = juce::File();
}

int ProjectSession::clean_up(const juce::Array<juce::File>& unused)
{
	const auto deleted = delete_media(project, unused);
	edits.clear();

	if (is_saved())
		remember_cleanup_hint(file, 0);

	return deleted;
}
