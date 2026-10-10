#pragma once

#include "engine/passthrough.h"

#include <juce_core/juce_core.h>

#include <map>

using HardwareControlValues = std::map<juce::String, int>;

struct Settings
{
	static constexpr int DEFAULT_JACK_BUFFER_SIZE = 128;

	static Settings load();
	static juce::File file();
	void save() const;

	juce::String device_type;
	juce::String output_device;
	juce::String input_device;
	double sample_rate = 0.0;
	int buffer_size = 0;
	int jack_buffer_size = DEFAULT_JACK_BUFFER_SIZE;

	bool test_monitor = false;
	InputRouting input_routing = InputRouting::left_to_both;

	std::map<juce::String, HardwareControlValues> hardware_controls;
};
