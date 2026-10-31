#pragma once

#include "engine/plugins/builtin/blues_drive.h"
#include "engine/plugins/builtin/cabinet.h"
#include "engine/plugins/builtin/distortion.h"
#include "engine/plugins/builtin/fuzz.h"
#include "engine/plugins/builtin/graphic_eq.h"
#include "engine/plugins/builtin/noise_gate.h"
#include "engine/plugins/builtin/overdrive.h"

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

inline const std::array<BuiltinPlugin, 7> BUILTIN_PLUGINS{{
	{"noise-gate", "Noise Gate", [] { return std::make_shared<NoiseGate>(); }},
	{"overdrive", "Overdrive", [] { return std::make_shared<Overdrive>(); }},
	{"blues-drive", "Blues Drive", [] { return std::make_shared<BluesDrive>(); }},
	{"distortion", "Distortion", [] { return std::make_shared<Distortion>(); }},
	{"fuzz", "Fuzz", [] { return std::make_shared<Fuzz>(); }},
	{"cabinet", "Cabinet", [] { return std::make_shared<Cabinet>(); }},
	{"graphic-eq", "Graphic Eq", [] { return std::make_shared<GraphicEq>(); }},
}};

inline std::shared_ptr<PluginProcessor> create_builtin_plugin(const juce::String& identifier)
{
	for (const auto& plugin : BUILTIN_PLUGINS)
		if (identifier == plugin.identifier)
			return plugin.create();

	return nullptr;
}
