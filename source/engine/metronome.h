#pragma once

#include "engine/musical_time.h"

#include <cstdint>

class Metronome
{
public:
	void prepare(double new_sample_rate);
	void reset();
	void render(float* const* outputs, int output_count, int sample_count, std::int64_t block_start, const Tempo& tempo);

private:
	void start_click(bool accent);
	float next_click_sample();

	double sample_rate = 0.0;
	int click_samples_left = 0;
	double phase = 0.0;
	double phase_step = 0.0;
	float level = 0.0f;
	float decay = 0.0f;
};
