#include "core/project/recent_projects.h"

#include "platform/system.h"

namespace
{
	constexpr auto FILE_NAME = "recent.json";

	juce::File recent_file()
	{
		return app_state_directory().getChildFile(FILE_NAME);
	}
}

RecentProjects RecentProjects::load()
{
	RecentProjects recent;
	const auto json = juce::JSON::parse(recent_file());

	if (const auto* paths = json.getArray())
		for (const auto& path : *paths)
			if (juce::File::isAbsolutePath(path.toString()) && recent.files.size() < MAX_FILES)
				recent.files.addIfNotAlreadyThere(juce::File(path.toString()));

	return recent;
}

void RecentProjects::add(const juce::File& file)
{
	files.removeAllInstancesOf(file);
	files.insert(0, file);
	files.resize(std::min(files.size(), MAX_FILES));
	save();
}

void RecentProjects::remove(const juce::File& file)
{
	files.removeAllInstancesOf(file);
	save();
}

void RecentProjects::save() const
{
	juce::Array<juce::var> paths;

	for (const auto& file : files)
		paths.add(file.getFullPathName());

	const auto target = recent_file();
	target.getParentDirectory().createDirectory();
	target.replaceWithText(juce::JSON::toString(paths));
}
