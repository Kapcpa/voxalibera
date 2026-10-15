#include "core/plugins/plugin_library.h"

#include "core/log.h"
#include "engine/plugins/builtin_plugins.h"
#include "platform/system.h"

namespace
{
	constexpr auto LIBRARY_FILE = "plugins.xml";

	bool matches(const Plugin& plugin, const juce::String& text)
	{
		return text.isEmpty() || plugin.name.containsIgnoreCase(text) || plugin.manufacturer.containsIgnoreCase(text) || plugin.format.containsIgnoreCase(text);
	}

	Plugin builtin_plugin(const BuiltinPlugin& builtin)
	{
		Plugin plugin;
		plugin.format = BUILTIN_FORMAT;
		plugin.identifier = builtin.identifier;
		plugin.name = builtin.name;
		plugin.manufacturer = BUILTIN_MANUFACTURER;
		plugin.version = JUCE_APPLICATION_VERSION_STRING;
		return plugin;
	}
}

PluginLibrary::PluginLibrary()
{
	juce::addDefaultFormatsToManager(formats);

	if (!file().existsAsFile())
		return;

	if (const auto xml = juce::parseXML(file()))
		known.recreateFromXml(*xml);
	else
		log_event("Plugin list unreadable, plugins need a new scan: " + file().getFullPathName());
}

juce::File PluginLibrary::file()
{
	return app_cache_directory().getChildFile(LIBRARY_FILE);
}

void PluginLibrary::save() const
{
	file().getParentDirectory().createDirectory();

	if (const auto xml = known.createXml())
		file().replaceWithText(xml->toString());
}

juce::StringArray PluginLibrary::default_folders() const
{
	juce::StringArray folders;

	for (auto* format : formats.getFormats()) {
		if (!format->canScanForPlugins())
			continue;

		const auto locations = format->getDefaultLocationsToSearch();

		for (int i = 0; i < locations.getNumPaths(); ++i)
			folders.addIfNotAlreadyThere(locations.getRawString(i));
	}

	return folders;
}

std::vector<Plugin> PluginLibrary::search(const juce::String& text) const
{
	std::vector<Plugin> found;

	for (const auto& builtin : BUILTIN_PLUGINS)
		if (const auto plugin = builtin_plugin(builtin); matches(plugin, text))
			found.push_back(plugin);

	for (const auto& description : known.getTypes())
		if (const auto plugin = plugin_from(description); !description.isInstrument && matches(plugin, text))
			found.push_back(plugin);

	std::stable_sort(found.begin(), found.end(), [](const Plugin& first, const Plugin& second) { return first.name.compareNatural(second.name) < 0; });
	return found;
}

std::unique_ptr<juce::PluginDescription> PluginLibrary::find(const Plugin& plugin) const
{
	if (plugin.format == BUILTIN_FORMAT)
		return nullptr;

	return known.getTypeForIdentifierString(plugin.identifier);
}

Plugin plugin_from(const juce::PluginDescription& description)
{
	Plugin plugin;
	plugin.format = description.pluginFormatName;
	plugin.identifier = description.createIdentifierString();
	plugin.name = description.name;
	plugin.manufacturer = description.manufacturerName;
	plugin.version = description.version;
	return plugin;
}
