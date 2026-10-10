#include "core/settings.h"

#include "core/log.h"
#include "platform/system.h"

namespace
{
	constexpr auto SETTINGS_FILE = "settings.json";

	const std::map<InputRouting, juce::String> ROUTING_NAMES{
		{InputRouting::stereo, "stereo"},
		{InputRouting::left_to_both, "left-to-both"},
		{InputRouting::right_to_both, "right-to-both"},
	};

	juce::String routing_name(InputRouting routing)
	{
		return ROUTING_NAMES.at(routing);
	}

	InputRouting routing_from_name(const juce::String& name, InputRouting fallback)
	{
		for (const auto& [routing, routing_text] : ROUTING_NAMES)
			if (routing_text == name)
				return routing;

		return fallback;
	}

	juce::var read(const juce::DynamicObject& object, const char* key, const juce::var& fallback)
	{
		return object.hasProperty(key) ? object.getProperty(key) : fallback;
	}

	std::map<juce::String, HardwareControlValues> read_hardware_controls(const juce::var& json)
	{
		std::map<juce::String, HardwareControlValues> cards;
		const auto* card_objects = json.getDynamicObject();

		if (card_objects == nullptr)
			return cards;

		for (const auto& card : card_objects->getProperties())
			if (const auto* values = card.value.getDynamicObject())
				for (const auto& value : values->getProperties())
					cards[card.name.toString()][value.name.toString()] = (int) value.value;

		return cards;
	}

	juce::var write_hardware_controls(const std::map<juce::String, HardwareControlValues>& cards)
	{
		juce::DynamicObject::Ptr card_objects = new juce::DynamicObject();

		for (const auto& [card_name, values] : cards) {
			juce::DynamicObject::Ptr value_objects = new juce::DynamicObject();

			for (const auto& [key, value] : values)
				value_objects->setProperty(key, value);

			card_objects->setProperty(card_name, value_objects.get());
		}

		return card_objects.get();
	}
}

juce::File Settings::file()
{
	return app_config_directory().getChildFile(SETTINGS_FILE);
}

Settings Settings::load()
{
	Settings settings;
	const auto json = juce::JSON::parse(file());
	const auto* root = json.getDynamicObject();

	if (root == nullptr) {
		if (file().existsAsFile())
			log_event("Settings file unreadable, using defaults: " + file().getFullPathName());

		return settings;
	}

	settings.device_type = read(*root, "device-type", "").toString();
	settings.output_device = read(*root, "output-device", "").toString();
	settings.input_device = read(*root, "input-device", "").toString();
	settings.sample_rate = read(*root, "sample-rate", settings.sample_rate);
	settings.buffer_size = read(*root, "buffer-size", settings.buffer_size);
	settings.jack_buffer_size = read(*root, "jack-buffer-size", settings.jack_buffer_size);
	settings.test_monitor = read(*root, "test-monitor", settings.test_monitor);
	settings.input_routing = routing_from_name(read(*root, "input-routing", "").toString(), settings.input_routing);
	settings.hardware_controls = read_hardware_controls(read(*root, "hardware-controls", {}));
	return settings;
}

void Settings::save() const
{
	juce::DynamicObject::Ptr root = new juce::DynamicObject();
	root->setProperty("device-type", device_type);
	root->setProperty("output-device", output_device);
	root->setProperty("input-device", input_device);
	root->setProperty("sample-rate", sample_rate);
	root->setProperty("buffer-size", buffer_size);
	root->setProperty("jack-buffer-size", jack_buffer_size);
	root->setProperty("test-monitor", test_monitor);
	root->setProperty("input-routing", routing_name(input_routing));
	root->setProperty("hardware-controls", write_hardware_controls(hardware_controls));

	file().getParentDirectory().createDirectory();
	file().replaceWithText(juce::JSON::toString(root.get()));
}
