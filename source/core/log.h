#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_core/juce_core.h>

#include <memory>

std::unique_ptr<juce::FileLogger> create_file_logger(const juce::String& first_line);
juce::File log_file();
void log_event(const juce::String& message);

class XrunWatch
{
public:
	void update(juce::AudioIODevice* device, double now);

private:
	juce::AudioIODevice* watched_device = nullptr;
	int logged_count = 0;
	double next_log_time = 0.0;
};
