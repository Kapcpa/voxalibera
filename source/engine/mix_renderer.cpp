#include "engine/mix_renderer.h"

namespace
{
	constexpr int LEFT = 0;
	constexpr int RIGHT = 1;
	constexpr float MONO_SUM_GAIN = 0.5f;
}

void MixRenderer::render(const EngineSnapshot& snapshot, Inputs inputs, float* const* outputs, int output_count, int sample_count, Transport::Block block)
{
	for (int offset = 0; offset < sample_count; offset += CHUNK_SAMPLES) {
		const auto chunk = std::min(CHUNK_SAMPLES, sample_count - offset);
		const Transport::Block chunk_block{block.playing, block.recording, block.start + offset};

		render_chunk(snapshot, inputs, offset, chunk, chunk_block);
		write_outputs(outputs, output_count, offset, chunk);
	}
}

void MixRenderer::render_chunk(const EngineSnapshot& snapshot, Inputs inputs, int offset, int sample_count, Transport::Block block)
{
	master_buffer.clear(0, sample_count);

	for (const auto& track : snapshot.tracks)
		render_track(track, inputs, offset, sample_count, block);

	master_buffer.applyGain(0, sample_count, snapshot.master_gain);
	master_meter.measure(master_buffer.getArrayOfReadPointers(), master_buffer.getNumChannels(), sample_count);
}

void MixRenderer::render_track(const TrackPlayback& track, Inputs inputs, int offset, int sample_count, Transport::Block block)
{
	track_buffer.clear(0, sample_count);

	if (block.playing)
		for (const auto& clip : track.clips)
			add_clip(track, clip, block.start, sample_count);

	if (track.monitors_input)
		add_input(track, inputs, offset, sample_count);

	if (track.channels == 1)
		track_buffer.copyFrom(RIGHT, 0, track_buffer, LEFT, 0, sample_count);

	track_buffer.applyGain(LEFT, 0, sample_count, track.audible ? track.gains.left : 0.0f);
	track_buffer.applyGain(RIGHT, 0, sample_count, track.audible ? track.gains.right : 0.0f);

	if (track.meter != NO_METER && track.meter < (int) MAX_TRACK_METERS)
		track_meters[(size_t) track.meter].measure(track_buffer.getArrayOfReadPointers(), track_buffer.getNumChannels(), sample_count);

	master_buffer.addFrom(LEFT, 0, track_buffer, LEFT, 0, sample_count);
	master_buffer.addFrom(RIGHT, 0, track_buffer, RIGHT, 0, sample_count);
}

void MixRenderer::add_clip(const TrackPlayback& track, const ClipPlayback& clip, std::int64_t position, int sample_count)
{
	const auto from = std::max(position, clip.start);
	const auto to = std::min(position + sample_count, clip.start + clip.length);

	if (from >= to)
		return;

	const auto destination = (int) (from - position);
	const auto count = (int) (to - from);
	const auto file_channels = clip.file->channels();
	float* const targets[]{clip_buffer.getWritePointer(LEFT, destination), clip_buffer.getWritePointer(RIGHT, destination)};

	clip.file->read(targets, clip.source_offset + (from - clip.start), count);

	if (file_channels == 1) {
		track_buffer.addFrom(LEFT, destination, clip_buffer, LEFT, destination, count, clip.gain);

		if (track.channels == 2)
			track_buffer.addFrom(RIGHT, destination, clip_buffer, LEFT, destination, count, clip.gain);

		return;
	}

	if (track.channels == 1) {
		track_buffer.addFrom(LEFT, destination, clip_buffer, LEFT, destination, count, clip.gain * MONO_SUM_GAIN);
		track_buffer.addFrom(LEFT, destination, clip_buffer, RIGHT, destination, count, clip.gain * MONO_SUM_GAIN);
		return;
	}

	track_buffer.addFrom(LEFT, destination, clip_buffer, LEFT, destination, count, clip.gain);
	track_buffer.addFrom(RIGHT, destination, clip_buffer, RIGHT, destination, count, clip.gain);
}

void MixRenderer::add_input(const TrackPlayback& track, Inputs inputs, int offset, int sample_count)
{
	for (int channel = 0; channel < track.channels; ++channel) {
		const auto input = track.first_input + channel;

		if (input >= 0 && input < inputs.count)
			track_buffer.addFrom(channel, 0, inputs.channels[input] + offset, sample_count);
	}
}

void MixRenderer::write_outputs(float* const* outputs, int output_count, int offset, int sample_count)
{
	for (int channel = 0; channel < output_count; ++channel) {
		auto* output = outputs[channel] + offset;

		if (output_count == 1) {
			juce::FloatVectorOperations::copyWithMultiply(output, master_buffer.getReadPointer(LEFT), MONO_SUM_GAIN, sample_count);
			juce::FloatVectorOperations::addWithMultiply(output, master_buffer.getReadPointer(RIGHT), MONO_SUM_GAIN, sample_count);
		} else if (channel < master_buffer.getNumChannels()) {
			juce::FloatVectorOperations::copy(output, master_buffer.getReadPointer(channel), sample_count);
		} else {
			juce::FloatVectorOperations::clear(output, sample_count);
		}
	}
}
