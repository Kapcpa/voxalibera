#pragma once

#include <juce_audio_devices/juce_audio_devices.h>

class XrunWatch
{
public:
	void update(juce::AudioIODevice* device, double now);

private:
	juce::AudioIODevice* watched_device = nullptr;
	int logged_count = 0;
	double next_log_time = 0.0;
};
