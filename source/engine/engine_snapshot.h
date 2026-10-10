#pragma once

#include "engine/audio_file.h"
#include "engine/levels.h"
#include "engine/musical_time.h"

#include <cstdint>
#include <memory>
#include <vector>

constexpr int NO_METER = -1;

struct ClipPlayback
{
	std::shared_ptr<const AudioFile> file;
	std::int64_t start = 0;
	std::int64_t source_offset = 0;
	std::int64_t length = 0;
	float gain = 1.0f;
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
