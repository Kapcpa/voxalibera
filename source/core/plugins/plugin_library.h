#pragma once

#include "core/project/model.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <memory>
#include <vector>

class PluginLibrary
{
public:
	PluginLibrary();

	static juce::File file();
	void save() const;
	juce::StringArray default_folders() const;

	std::vector<Plugin> search(const juce::String& text) const;
	std::unique_ptr<juce::PluginDescription> find(const Plugin& plugin) const;

	juce::AudioPluginFormatManager formats;
	juce::KnownPluginList known;
};

Plugin plugin_from(const juce::PluginDescription& description);
