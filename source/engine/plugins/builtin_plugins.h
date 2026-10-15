#pragma once

#include "engine/plugins/plugin_processor.h"

#include <array>
#include <cmath>
#include <functional>
#include <memory>

class TestDriveProcessor : public PluginProcessor
{
public:
	void prepare(const PluginProcessContext&) override {}

	void process(juce::AudioBuffer<float>& buffer, int sample_count, const PluginPlayhead&) override
	{
		for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
			auto* samples = buffer.getWritePointer(channel);

			for (int index = 0; index < sample_count; ++index)
				samples[index] = std::tanh(samples[index] * DRIVE);
		}
	}

private:
	static constexpr float DRIVE = 2.5f;
};

struct BuiltinPlugin
{
	const char* identifier;
	const char* name;
	std::function<std::shared_ptr<PluginProcessor>()> create;
};

inline constexpr auto BUILTIN_FORMAT = "builtin";
inline constexpr auto BUILTIN_MANUFACTURER = "Voxalibera";

inline const std::array<BuiltinPlugin, 1> BUILTIN_PLUGINS{{
	{"test-drive", "Test Drive", [] { return std::make_shared<TestDriveProcessor>(); }},
}};

inline std::shared_ptr<PluginProcessor> create_builtin_plugin(const juce::String& identifier)
{
	for (const auto& plugin : BUILTIN_PLUGINS)
		if (identifier == plugin.identifier)
			return plugin.create();

	return nullptr;
}
