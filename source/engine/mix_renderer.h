#pragma once

#include "engine/engine_snapshot.h"
#include "engine/level_meter.h"
#include "engine/transport.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <array>

class MixRenderer
{
public:
	static constexpr int CHUNK_SAMPLES = 2048;
	static constexpr size_t MAX_TRACK_METERS = 256;

	struct Inputs
	{
		const float* const* channels = nullptr;
		int count = 0;
	};

	void render(const EngineSnapshot& snapshot, Inputs inputs, float* const* outputs, int output_count, int sample_count, Transport::Block block);

	std::array<LevelMeter, MAX_TRACK_METERS> track_meters;
	LevelMeter master_meter;

private:
	void render_chunk(const EngineSnapshot& snapshot, Inputs inputs, int offset, int sample_count, Transport::Block block);
	void render_track(const TrackPlayback& track, Inputs inputs, int offset, int sample_count, Transport::Block block);
	void add_clip(const TrackPlayback& track, const ClipPlayback& clip, std::int64_t position, int sample_count);
	void add_input(const TrackPlayback& track, Inputs inputs, int offset, int sample_count);
	void write_outputs(float* const* outputs, int output_count, int offset, int sample_count);

	juce::AudioBuffer<float> track_buffer{2, CHUNK_SAMPLES};
	juce::AudioBuffer<float> clip_buffer{2, CHUNK_SAMPLES};
	juce::AudioBuffer<float> master_buffer{2, CHUNK_SAMPLES};
};
