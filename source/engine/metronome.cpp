#include "engine/metronome.h"

#include <cmath>

namespace
{
	constexpr double ACCENT_FREQUENCY = 1760.0;
	constexpr double BEAT_FREQUENCY = 1320.0;
	constexpr float ACCENT_LEVEL = 0.5f;
	constexpr float BEAT_LEVEL = 0.35f;
	constexpr double CLICK_SECONDS = 0.04;
	constexpr double DECAY_TIME_CONSTANT_SECONDS = 0.008;
}

void Metronome::prepare(double new_sample_rate)
{
	sample_rate = new_sample_rate;
	reset();
}

void Metronome::reset()
{
	click_samples_left = 0;
	level = 0.0f;
}

void Metronome::start_click(bool accent)
{
	click_samples_left = (int) (CLICK_SECONDS * sample_rate);
	phase = 0.0;
	phase_step = juce::MathConstants<double>::twoPi * (accent ? ACCENT_FREQUENCY : BEAT_FREQUENCY) / sample_rate;
	level = accent ? ACCENT_LEVEL : BEAT_LEVEL;
	decay = (float) std::exp(-1.0 / (DECAY_TIME_CONSTANT_SECONDS * sample_rate));
}

float Metronome::next_click_sample()
{
	if (click_samples_left <= 0)
		return 0.0f;

	const auto value = level * (float) std::sin(phase);
	phase += phase_step;
	level *= decay;
	--click_samples_left;
	return value;
}

void Metronome::render(float* const* outputs, int output_count, int sample_count, std::int64_t block_start, const Tempo& tempo)
{
	if (sample_rate <= 0.0)
		return;

	const auto spacing = samples_per_beat(tempo, sample_rate);
	auto beat_index = (std::int64_t) std::ceil((double) block_start / spacing);
	auto next_beat = beat_start_sample(beat_index, tempo, sample_rate);

	while (next_beat < block_start)
		next_beat = beat_start_sample(++beat_index, tempo, sample_rate);

	for (int offset = 0; offset < sample_count; ++offset) {
		if (block_start + offset == next_beat) {
			start_click(beat_index % tempo.signature.beats_per_bar == 0);
			next_beat = beat_start_sample(++beat_index, tempo, sample_rate);
		}

		const auto value = next_click_sample();

		for (int channel = 0; channel < output_count; ++channel)
			outputs[channel][offset] += value;
	}
}
