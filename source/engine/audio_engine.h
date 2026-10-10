#pragma once

#include "engine/engine_snapshot.h"
#include "engine/level_meter.h"
#include "engine/metronome.h"
#include "engine/mix_renderer.h"
#include "engine/passthrough.h"
#include "engine/recording_session.h"
#include "engine/snapshot_exchange.h"
#include "engine/transport.h"

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
	MixRenderer mixer;
	Transport transport;
	SnapshotExchange<const EngineSnapshot> snapshots;
	SnapshotExchange<RecordingSession> recording;
	std::atomic<double> sample_rate{0.0};
	std::atomic<bool> running{false};
	std::atomic<bool> running_realtime{false};
	std::atomic<int> device_starts{0};

private:
	std::atomic<bool> audio_thread_prepared{false};
	Metronome metronome;
};
