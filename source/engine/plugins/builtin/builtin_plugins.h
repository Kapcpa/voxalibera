#pragma once

#include "engine/plugins/builtin/test_drive.h"

#include <array>
#include <functional>
#include <memory>

struct BuiltinPlugin
{
	const char* identifier;
	const char* name;
	std::function<std::shared_ptr<PluginProcessor>()> create;
};

inline constexpr auto BUILTIN_FORMAT = "builtin";
inline constexpr auto BUILTIN_MANUFACTURER = "Voxalibera";

inline const std::array<BuiltinPlugin, 1> BUILTIN_PLUGINS{{
	{"test-drive", "Test Drive", [] { return std::make_shared<TestDrive>(); }},
}};

inline std::shared_ptr<PluginProcessor> create_builtin_plugin(const juce::String& identifier)
{
	for (const auto& plugin : BUILTIN_PLUGINS)
		if (identifier == plugin.identifier)
			return plugin.create();

	return nullptr;
}
