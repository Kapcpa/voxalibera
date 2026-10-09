#pragma once

#include "engine/level_meter.h"
#include "engine/passthrough.h"

#include <juce_audio_devices/juce_audio_devices.h>

#include <atomic>

class AudioEngine : public juce::AudioIODeviceCallback
{
public:
	AudioEngine();
	~AudioEngine() override;

	void audioDeviceIOCallbackWithContext(const float* const* inputs, int input_count, float* const* outputs, int output_count, int sample_count, const juce::AudioIODeviceCallbackContext& context) override;
	void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
	void audioDeviceStopped() override;

	juce::AudioDeviceManager device_manager;
	Passthrough passthrough;
	LevelMeter input_meter;
	std::atomic<bool> running_realtime{false};
	std::atomic<int> device_starts{0};

private:
	std::atomic<bool> audio_thread_prepared{false};
};
