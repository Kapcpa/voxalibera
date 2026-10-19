#include "core/media/impulse_import.h"

#include "core/media/project_folders.h"

#include <juce_audio_formats/juce_audio_formats.h>

juce::String impulse_wildcard()
{
	juce::AudioFormatManager formats;
	formats.registerBasicFormats();
	return formats.getWildcardForAllFormats();
}

juce::String import_impulse_file(Project& project, const juce::File& file)
{
	if (project.folder == juce::File())
		project.folder = new_unsaved_project_folder();

	const auto folder = project.folder.getChildFile(IMPULSES_FOLDER);

	if (file.isAChildOf(folder))
		return relative_media_path(project.folder, file);

	if (const auto created = folder.createDirectory(); created.failed())
		return {};

	const auto target = folder.getNonexistentChildFile(file.getFileNameWithoutExtension(), file.getFileExtension());
	return file.copyFileTo(target) ? relative_media_path(project.folder, target) : juce::String();
}
