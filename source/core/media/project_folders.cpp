#include "core/media/project_folders.h"

#include "platform/system.h"

namespace
{
	constexpr auto UNSAVED_FOLDER = "unsaved";
	constexpr auto FOLDER_TIME_FORMAT = "%Y-%m-%d_%H-%M-%S";
	constexpr auto TAKE_EXTENSION = ".wav";
	constexpr auto FALLBACK_TRACK_NAME = "track";
	const juce::String ALLOWED_NAME_CHARACTERS = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_";

	juce::String file_safe(const juce::String& name)
	{
		const auto safe = name.trim().replaceCharacter(' ', '-').retainCharacters(ALLOWED_NAME_CHARACTERS);
		return safe.isNotEmpty() ? safe : juce::String(FALLBACK_TRACK_NAME);
	}

	juce::File unsaved_projects_folder()
	{
		return app_data_directory().getChildFile(UNSAVED_FOLDER);
	}
}

juce::File new_unsaved_project_folder()
{
	const auto folder = unsaved_projects_folder().getChildFile(juce::Time::getCurrentTime().formatted(FOLDER_TIME_FORMAT));
	folder.getChildFile(AUDIO_FOLDER).createDirectory();
	return folder;
}

bool is_unsaved_project_folder(const juce::File& folder)
{
	return folder.isAChildOf(unsaved_projects_folder());
}

juce::File next_take_file(const juce::File& project_folder, const juce::String& track_name)
{
	const auto audio_folder = project_folder.getChildFile(AUDIO_FOLDER);
	const auto base = file_safe(track_name) + "-take-";

	for (int number = 1;; ++number) {
		const auto candidate = audio_folder.getChildFile(base + juce::String(number) + TAKE_EXTENSION);

		if (!candidate.exists())
			return candidate;
	}
}

juce::String relative_media_path(const juce::File& project_folder, const juce::File& file)
{
	return file.getRelativePathFrom(project_folder);
}

juce::String media_display_name(const juce::String& relative_path)
{
	return relative_path.fromLastOccurrenceOf("/", false, false).upToLastOccurrenceOf(".", false, false);
}

juce::Array<juce::File> unsaved_project_folders()
{
	return unsaved_projects_folder().findChildFiles(juce::File::findDirectories, false);
}

juce::Array<juce::File> audio_files_in(const juce::File& project_folder)
{
	auto files = project_folder.getChildFile(AUDIO_FOLDER).findChildFiles(juce::File::findFiles, false, juce::String("*") + TAKE_EXTENSION);
	files.sort();
	return files;
}

juce::Array<juce::File> impulse_files_in(const juce::File& project_folder)
{
	auto files = project_folder.getChildFile(IMPULSES_FOLDER).findChildFiles(juce::File::findFiles, false);
	files.sort();
	return files;
}
