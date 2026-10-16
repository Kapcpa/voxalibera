#pragma once

#include "engine/plugins/plugin_processor.h"

#include <atomic>
#include <memory>
#include <optional>
#include <vector>

struct BuiltinParameterInfo
{
	juce::String key;
	juce::String name;
	float minimum = 0.0f;
	float maximum = 1.0f;
	float default_value = 0.0f;
	juce::String unit;
	int decimals = 1;
	std::optional<float> centre = std::nullopt;
};

class BuiltinParameter
{
public:
	explicit BuiltinParameter(BuiltinParameterInfo info);

	float value() const { return current.load(std::memory_order_relaxed); }

	const BuiltinParameterInfo info;

private:
	friend class BuiltinProcessor;

	void store(float value);

	std::atomic<float> current;
};

class BuiltinProcessor : public PluginProcessor
{
public:
	const std::vector<std::unique_ptr<BuiltinParameter>>& parameters() const { return parameter_list; }
	void set_parameter(BuiltinParameter& parameter, float value);

	juce::MemoryBlock get_state() const override;
	void set_state(const juce::MemoryBlock& state) override;

protected:
	BuiltinParameter& add_parameter(BuiltinParameterInfo info);

private:
	std::vector<std::unique_ptr<BuiltinParameter>> parameter_list;
};
