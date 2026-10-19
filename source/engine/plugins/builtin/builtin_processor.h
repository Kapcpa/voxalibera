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
	juce::String unit = {};
	int decimals = 1;
	std::optional<float> centre = std::nullopt;
	juce::StringArray choices = {};
};

class BuiltinParameter
{
public:
	explicit BuiltinParameter(BuiltinParameterInfo info);

	float value() const { return current.load(std::memory_order_relaxed); }
	bool is_choice() const { return !info.choices.isEmpty(); }

	const BuiltinParameterInfo info;

private:
	friend class BuiltinProcessor;

	void store(float value);

	std::atomic<float> current;
};

struct BuiltinFileInfo
{
	juce::String key;
	juce::String name;
	juce::String empty_text;
};

class BuiltinFile
{
public:
	explicit BuiltinFile(BuiltinFileInfo info) : info(std::move(info)) {}

	const juce::String& path() const { return relative_path; }

	const BuiltinFileInfo info;

private:
	friend class BuiltinProcessor;

	juce::String relative_path;
};

class BuiltinProcessor : public PluginProcessor
{
public:
	const std::vector<std::unique_ptr<BuiltinParameter>>& parameters() const { return parameter_list; }
	void set_parameter(BuiltinParameter& parameter, float value);

	const std::vector<std::unique_ptr<BuiltinFile>>& files() const { return file_list; }
	void set_file(BuiltinFile& file, const juce::String& relative_path);
	juce::File resolve(const BuiltinFile& file) const;
	void use_media_folder(const juce::File& folder) { media_folder = folder; }

	juce::MemoryBlock get_state() const override;
	void set_state(const juce::MemoryBlock& state) override;

	static juce::StringArray files_in_state(const juce::MemoryBlock& state);

protected:
	BuiltinParameter& add_parameter(BuiltinParameterInfo info);
	BuiltinFile& add_file(BuiltinFileInfo info);
	virtual void file_changed(BuiltinFile&) {}

private:
	std::vector<std::unique_ptr<BuiltinParameter>> parameter_list;
	std::vector<std::unique_ptr<BuiltinFile>> file_list;
	juce::File media_folder;
};
