#include "ui/device_selector_view.h"

namespace
{
	constexpr int MIN_CHANNELS = 1;
	constexpr int MAX_CHANNELS = 2;
	constexpr bool SHOW_MIDI_INPUTS = false;
	constexpr bool SHOW_MIDI_OUTPUTS = false;
	constexpr bool CHANNELS_AS_STEREO_PAIRS = false;
	constexpr bool HIDE_ADVANCED_OPTIONS = false;
}

DeviceSelectorView::DeviceSelectorView(juce::AudioDeviceManager& device_manager) : AudioDeviceSelectorComponent(device_manager, MIN_CHANNELS, MAX_CHANNELS, MIN_CHANNELS, MAX_CHANNELS, SHOW_MIDI_INPUTS, SHOW_MIDI_OUTPUTS, CHANNELS_AS_STEREO_PAIRS, HIDE_ADVANCED_OPTIONS)
{
}
