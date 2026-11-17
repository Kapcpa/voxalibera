#include "engine/audio_engine.h"

#include "platform/system.h"

#include <limits>

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

	const auto* snapshot = snapshots.begin_block();
	auto* session = recording.begin_block();

	input_meter.measure(inputs, input_count, sample_count);

	for (int done = 0; done < sample_count;) {
		const auto block = transport.advance(sample_count - done);
		render_segment(snapshot, session, {inputs, input_count}, outputs, output_count, done, block);
		done += block.length;
	}

	recording.end_block();
	passthrough.add_to(inputs, input_count, outputs, output_count, sample_count);
	snapshots.end_block();
}

void AudioEngine::render_segment(const EngineSnapshot* snapshot, RecordingSession* session, MixRenderer::Inputs inputs, float* const* outputs, int output_count, int offset, const Transport::Block& block)
{
	if (session != nullptr && block.recording)
		session->write(inputs.channels, inputs.count, offset, block.length, block.start, block.record_from);

	if (snapshot == nullptr) {
		for (int channel = 0; channel < output_count; ++channel)
			juce::FloatVectorOperations::clear(outputs[channel] + offset, block.length);

		metronome.reset();
		return;
	}

	mixer.render(*snapshot, inputs, outputs, output_count, block.length, block, offset);

	const auto metronome_on = snapshot->metronome && block.playing;

	if (metronome_on || block.is_counting_in())
		metronome.render(outputs, output_count, offset, block.length, block.start, metronome_on ? std::numeric_limits<std::int64_t>::max() : block.record_from, snapshot->tempo);
	else
		metronome.reset();
}

void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
	metronome.prepare(device->getCurrentSampleRate());
	sample_rate = device->getCurrentSampleRate();
	running = true;
	audio_thread_prepared = false;
	running_realtime = false;
	++device_starts;
}

void AudioEngine::audioDeviceStopped()
{
	running = false;
	running_realtime = false;
}
