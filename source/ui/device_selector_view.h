#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

class DeviceSelectorView : public juce::AudioDeviceSelectorComponent
{
public:
	explicit DeviceSelectorView(juce::AudioDeviceManager& device_manager);
};
