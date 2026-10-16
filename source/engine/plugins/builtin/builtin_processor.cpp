#include "engine/plugins/builtin/builtin_processor.h"

#include <algorithm>

BuiltinParameter::BuiltinParameter(BuiltinParameterInfo parameter_info) : info(std::move(parameter_info)), current(info.default_value)
{
}

void BuiltinParameter::store(float value)
{
	current.store(std::clamp(value, info.minimum, info.maximum), std::memory_order_relaxed);
}

BuiltinParameter& BuiltinProcessor::add_parameter(BuiltinParameterInfo info)
{
	return *parameter_list.emplace_back(std::make_unique<BuiltinParameter>(std::move(info)));
}

void BuiltinProcessor::set_parameter(BuiltinParameter& parameter, float value)
{
	parameter.store(value);
	mark_changed();
}

juce::MemoryBlock BuiltinProcessor::get_state() const
{
	auto* values = new juce::DynamicObject();

	for (const auto& parameter : parameter_list)
		values->setProperty(parameter->info.key, parameter->value());

	const auto text = juce::JSON::toString(juce::var(values), true);
	return {text.toRawUTF8(), text.getNumBytesAsUTF8()};
}

void BuiltinProcessor::set_state(const juce::MemoryBlock& state)
{
	const auto values = juce::JSON::parse(state.toString());

	for (const auto& parameter : parameter_list)
		if (const auto& value = values[juce::Identifier(parameter->info.key)]; !value.isVoid())
			parameter->store((float) value);
}
