#pragma once

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
