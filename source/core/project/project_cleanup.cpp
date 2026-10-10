#include "core/project/project_cleanup.h"

#include "core/log.h"
#include "core/media/peaks.h"
#include "core/media/project_folders.h"
#include "platform/system.h"

#include <set>

namespace
{
	constexpr auto HINTS_FILE = "cleanup-hints.json";

	juce::File hints_file()
	{
		return app_state_directory().getChildFile(HINTS_FILE);
	}

	juce::var load_hints()
	{
		const auto hints = juce::JSON::parse(hints_file());
		return hints.getDynamicObject() != nullptr ? hints : juce::var(new juce::DynamicObject());
	}
}

juce::Array<juce::File> unused_media(const Project& project)
{
	std::set<juce::String> used;

	for (const auto& track : project.tracks)
		for (const auto& clip : track.clips)
			used.insert(project.folder.getChildFile(clip.file).getFullPathName());

	juce::Array<juce::File> unused;

	for (const auto& file : audio_files_in(project.folder))
		if (!used.contains(file.getFullPathName()))
			unused.add(file);

	return unused;
}

juce::int64 total_size(const juce::Array<juce::File>& files)
{
	juce::int64 bytes = 0;

	for (const auto& file : files)
		bytes += file.getSize();

	return bytes;
}

juce::String describe_size(juce::int64 bytes)
{
	return juce::File::descriptionOfSizeInBytes(bytes);
}

int delete_media(const Project& project, const juce::Array<juce::File>& files)
{
	int deleted = 0;

	for (const auto& file : files) {
		if (!file.isAChildOf(project.folder.getChildFile(AUDIO_FOLDER)) || !file.deleteFile())
			continue;

		peak_cache_file(project.folder, relative_media_path(project.folder, file)).deleteFile();
		++deleted;
	}

	log_event("Cleaned up " + juce::String(deleted) + " unused file(s) in " + project.folder.getFullPathName());
	return deleted;
}

bool should_hint_cleanup(const juce::File& project_file, juce::int64 unused_bytes)
{
	const auto hinted_at = (juce::int64) load_hints().getProperty(juce::Identifier(project_file.getFullPathName()), 0);
	return unused_bytes >= UNUSED_HINT_BYTES && unused_bytes >= hinted_at + UNUSED_HINT_BYTES;
}

void remember_cleanup_hint(const juce::File& project_file, juce::int64 unused_bytes)
{
	auto hints = load_hints();
	hints.getDynamicObject()->setProperty(juce::Identifier(project_file.getFullPathName()), unused_bytes);
	hints_file().getParentDirectory().createDirectory();
	hints_file().replaceWithText(juce::JSON::toString(hints));
}
