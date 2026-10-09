#include "app/device_listing.h"

#include <juce_audio_devices/juce_audio_devices.h>

#include <iostream>

namespace
{
void print_device_names(juce::AudioIODeviceType& type, bool inputs)
{
	for (const auto& name : type.getDeviceNames(inputs))
		std::cout << "  " << (inputs ? "in:  " : "out: ") << name << "\n";
}
}

void print_audio_devices()
{
	juce::AudioDeviceManager device_manager;

	for (auto* type : device_manager.getAvailableDeviceTypes())
	{
		type->scanForDevices();
		std::cout << type->getTypeName() << "\n";
		print_device_names(*type, false);
		print_device_names(*type, true);
	}
}
