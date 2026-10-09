#include "engine/audio_engine.h"

#include "platform/realtime_priority.h"

AudioEngine::AudioEngine()
{
	device_manager.addAudioCallback(this);
}

AudioEngine::~AudioEngine()
{
	device_manager.removeAudioCallback(this);
	device_manager.closeAudioDevice();
}

void AudioEngine::audioDeviceIOCallbackWithContext(const float* const* inputs, int input_count, float* const* outputs, int output_count, int sample_count, const juce::AudioIODeviceCallbackContext&)
{
	if (!audio_thread_prepared.exchange(true, std::memory_order_relaxed))
		running_realtime = make_current_thread_realtime();

	input_meter.measure(inputs, input_count, sample_count);
	passthrough.process(inputs, input_count, outputs, output_count, sample_count);
}

void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice*)
{
	audio_thread_prepared = false;
	running_realtime = false;
	++device_starts;
}

void AudioEngine::audioDeviceStopped()
{
	running_realtime = false;
}
