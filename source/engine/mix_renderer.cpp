#include "engine/mix_renderer.h"

#include "engine/levels.h"

#include <cmath>

namespace
{
	constexpr int LEFT = 0;
	constexpr int RIGHT = 1;
	constexpr float MONO_SUM_GAIN = 0.5f;

	std::int64_t wrapped_into_loop(std::int64_t position, const Transport::Loop& loop)
	{
		return position < loop.end ? position : loop.start + (position - loop.end) % loop.length();
	}

	std::int64_t position_ahead(const Transport::Block& block, int latency)
	{
		return block.wraps_ahead() ? wrapped_into_loop(block.start + latency, block.loop) : block.start + latency;
	}
}

void MixRenderer::render(const EngineSnapshot& snapshot, Inputs inputs, float* const* outputs, int output_count, int sample_count, Transport::Block block, int buffer_offset)
{
	for (int offset = 0; offset < sample_count; offset += CHUNK_SAMPLES) {
		const auto chunk = std::min(CHUNK_SAMPLES, sample_count - offset);
		auto chunk_block = block;
		chunk_block.start = block.start + offset;
		chunk_block.length = chunk;

		render_chunk(snapshot, inputs, buffer_offset + offset, chunk, chunk_block);
		write_outputs(outputs, output_count, buffer_offset + offset, chunk);
	}
}

void MixRenderer::render_chunk(const EngineSnapshot& snapshot, Inputs inputs, int offset, int sample_count, Transport::Block block)
{
	master_buffer.clear(0, sample_count);

	for (const auto& track : snapshot.tracks)
		render_track(track, inputs, offset, sample_count, block, snapshot);

	process_master(snapshot, sample_count, block);
	master_buffer.applyGain(0, sample_count, snapshot.master_gain);
	master_meter.measure(master_buffer.getArrayOfReadPointers(), master_buffer.getNumChannels(), sample_count);
}

void MixRenderer::process_master(const EngineSnapshot& snapshot, int sample_count, Transport::Block block)
{
	if (snapshot.master_plugins == nullptr)
		return;

	const auto source_position = position_ahead(block, snapshot.master_latency());
	snapshot.master_plugins->process(master_buffer, sample_count, {block.playing, block.recording, source_position, snapshot.tempo, block.wraps_ahead() ? block.loop : Transport::Loop{}});
}

void MixRenderer::render_track(const TrackPlayback& track, Inputs inputs, int offset, int sample_count, Transport::Block block, const EngineSnapshot& snapshot)
{
	const auto latency = (track.plugins != nullptr ? track.plugins->total_latency() : 0) + snapshot.master_latency();
	const auto source_position = position_ahead(block, latency);
	const auto& tempo = snapshot.tempo;
	const auto before_loop_end = block.wraps_ahead() && source_position < block.loop.end ? (int) std::min<std::int64_t>(sample_count, block.loop.end - source_position) : sample_count;

	track_buffer.clear(0, sample_count);

	if (block.playing) {
		add_clips(track, source_position, before_loop_end, 0);

		if (before_loop_end < sample_count)
			add_clips(track, block.loop.start, sample_count - before_loop_end, before_loop_end);
	}

	if (track.monitors_input)
		add_input(track, inputs, offset, sample_count);

	if (track.channels == 1)
		track_buffer.copyFrom(RIGHT, 0, track_buffer, LEFT, 0, sample_count);

	if (track.plugins != nullptr)
		track.plugins->process(track_buffer, sample_count, {block.playing, block.recording, source_position, tempo, block.wraps_ahead() ? block.loop : Transport::Loop{}});

	track_buffer.applyGain(LEFT, 0, sample_count, track.audible ? track.gains.left : 0.0f);
	track_buffer.applyGain(RIGHT, 0, sample_count, track.audible ? track.gains.right : 0.0f);

	if (track.meter != NO_METER && track.meter < (int) MAX_TRACK_METERS)
		track_meters[(size_t) track.meter].measure(track_buffer.getArrayOfReadPointers(), track_buffer.getNumChannels(), sample_count);

	master_buffer.addFrom(LEFT, 0, track_buffer, LEFT, 0, sample_count);
	master_buffer.addFrom(RIGHT, 0, track_buffer, RIGHT, 0, sample_count);
}

void MixRenderer::add_clips(const TrackPlayback& track, std::int64_t position, int sample_count, int destination)
{
	for (const auto& clip : track.clips)
		add_clip(track, clip, position, sample_count, destination);
}

void MixRenderer::add_clip(const TrackPlayback& track, const ClipPlayback& clip, std::int64_t position, int sample_count, int first_destination)
{
	const auto from = std::max(position, clip.start);
	const auto to = std::min(position + sample_count, clip.start + clip.length);

	if (from >= to)
		return;

	const auto destination = first_destination + (int) (from - position);
	const auto count = (int) (to - from);
	const auto file_channels = clip.file->channels();
	read_clip(clip, from, count, destination);
	apply_fades(clip, from, count, destination);

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

void MixRenderer::read_clip(const ClipPlayback& clip, std::int64_t from, int count, int destination)
{
	if (clip.resampler != nullptr) {
		read_resampled(clip, from, count, destination);
		return;
	}

	float* const targets[]{clip_buffer.getWritePointer(LEFT, destination), clip_buffer.getWritePointer(RIGHT, destination)};
	clip.file->read(targets, std::llround(clip.file_position(from)), count);
}

void MixRenderer::read_resampled(const ClipPlayback& clip, std::int64_t from, int count, int destination)
{
	const auto first = clip.file_position(from);
	const auto last = clip.file_position(from + count - 1);
	const auto half_taps = clip.resampler->half_taps();
	const auto lowest = (std::int64_t) std::floor(first) - half_taps + 1;
	const auto highest = (std::int64_t) std::floor(last) + half_taps;
	const auto needed = (int) std::min<std::int64_t>(highest - lowest + 1, RESAMPLE_SAMPLES);
	const auto readable_from = std::max<std::int64_t>(lowest, 0);
	const auto readable_to = std::min<std::int64_t>(lowest + needed, clip.file->length());

	resample_buffer.clear(0, needed);

	if (readable_from < readable_to) {
		const auto skip = (int) (readable_from - lowest);
		float* const targets[]{resample_buffer.getWritePointer(LEFT, skip), resample_buffer.getWritePointer(RIGHT, skip)};
		clip.file->read(targets, readable_from, (int) (readable_to - readable_from));
	}

	for (int channel = 0; channel < std::min(clip.file->channels(), 2); ++channel)
		clip.resampler->render(resample_buffer.getReadPointer(channel), first - (double) lowest, count, clip_buffer.getWritePointer(channel, destination));
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

void MixRenderer::apply_fades(const ClipPlayback& clip, std::int64_t from, int count, int destination)
{
	const auto end = clip.start + clip.length;

	if (from >= clip.start + clip.fade_in && from + count <= end - clip.fade_out)
		return;

	const auto channels = std::min(clip.file->channels(), clip_buffer.getNumChannels());
	const auto samples = clip_buffer.getArrayOfWritePointers();

	for (auto position = from; position < from + count; ++position) {
		const auto gain = clip_fade_gain(position - clip.start, clip.length, clip.fade_in, clip.fade_out);

		for (int channel = 0; channel < channels; ++channel)
			samples[channel][destination + (position - from)] *= gain;
	}
}
