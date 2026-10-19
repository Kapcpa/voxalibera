#include "engine/plugins/builtin/builtin_processor.h"

#include <algorithm>
#include <cmath>

namespace
{
	constexpr auto FILES_KEY = "files";
}

BuiltinParameter::BuiltinParameter(BuiltinParameterInfo parameter_info) : info(std::move(parameter_info)), current(info.default_value)
{
}

void BuiltinParameter::store(float value)
{
	const auto clamped = std::clamp(value, info.minimum, info.maximum);
	current.store(is_choice() ? std::round(clamped) : clamped, std::memory_order_relaxed);
}

BuiltinParameter& BuiltinProcessor::add_parameter(BuiltinParameterInfo info)
{
	if (!info.choices.isEmpty()) {
		info.minimum = 0.0f;
		info.maximum = (float) (info.choices.size() - 1);
		info.decimals = 0;
	}

	return *parameter_list.emplace_back(std::make_unique<BuiltinParameter>(std::move(info)));
}

BuiltinFile& BuiltinProcessor::add_file(BuiltinFileInfo info)
{
	return *file_list.emplace_back(std::make_unique<BuiltinFile>(std::move(info)));
}

void BuiltinProcessor::set_parameter(BuiltinParameter& parameter, float value)
{
	parameter.store(value);
	mark_changed();
}

void BuiltinProcessor::set_file(BuiltinFile& file, const juce::String& relative_path)
{
	file.relative_path = relative_path;
	file_changed(file);
	mark_changed();
}

juce::File BuiltinProcessor::resolve(const BuiltinFile& file) const
{
	if (file.relative_path.isEmpty() || media_folder == juce::File())
		return {};

	return media_folder.getChildFile(file.relative_path);
}

juce::MemoryBlock BuiltinProcessor::get_state() const
{
	auto* values = new juce::DynamicObject();

	for (const auto& parameter : parameter_list)
		values->setProperty(parameter->info.key, parameter->value());

	if (!file_list.empty()) {
		auto* paths = new juce::DynamicObject();

		for (const auto& file : file_list)
			paths->setProperty(file->info.key, file->relative_path);

		values->setProperty(FILES_KEY, juce::var(paths));
	}

	const auto text = juce::JSON::toString(juce::var(values), true);
	return {text.toRawUTF8(), text.getNumBytesAsUTF8()};
}

void BuiltinProcessor::set_state(const juce::MemoryBlock& state)
{
	const auto values = juce::JSON::parse(state.toString());

	for (const auto& parameter : parameter_list)
		if (const auto& value = values[juce::Identifier(parameter->info.key)]; !value.isVoid())
			parameter->store((float) value);

	const auto& paths = values[FILES_KEY];

	for (const auto& file : file_list) {
		file->relative_path = paths[juce::Identifier(file->info.key)].toString();
		file_changed(*file);
	}
}

juce::StringArray BuiltinProcessor::files_in_state(const juce::MemoryBlock& state)
{
	juce::StringArray found;

	if (const auto* paths = juce::JSON::parse(state.toString())[FILES_KEY].getDynamicObject())
		for (const auto& property : paths->getProperties())
			if (const auto path = property.value.toString(); path.isNotEmpty())
				found.add(path);

	return found;
}
