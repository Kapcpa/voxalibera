#pragma once

#include "engine/audio_file.h"
#include "engine/levels.h"
#include "engine/musical_time.h"
#include "engine/resampler.h"

#include <cstdint>
#include <memory>
#include <vector>

constexpr int NO_METER = -1;

struct ClipPlayback
{
	std::shared_ptr<const AudioFile> file;
	std::int64_t start = 0;
	std::int64_t length = 0;
	double file_start = 0.0;
	float gain = 1.0f;
	std::shared_ptr<const ResamplingKernel> resampler;

	double file_position(std::int64_t position) const { return file_start + (double) (position - start) * (resampler != nullptr ? resampler->step() : 1.0); }
};

struct TrackPlayback
{
	int meter = NO_METER;
	int channels = 1;
	int first_input = 0;
	bool monitors_input = false;
	bool audible = true;
	StereoGains gains;
	std::vector<ClipPlayback> clips;
};

struct EngineSnapshot
{
	Tempo tempo;
	bool metronome = false;
	float master_gain = 1.0f;
	std::vector<TrackPlayback> tracks;
};
