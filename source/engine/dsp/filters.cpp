#include "engine/dsp/filters.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <numbers>

namespace
{
	constexpr double HIGHEST_CUTOFF_RATIO = 0.49;
	constexpr float TAPER_DECADES = 2.0f;
	constexpr float LOWEST_Q = 0.01f;
	constexpr float LOWEST_MAGNITUDE = 1.0e-12f;

	double warped(double sample_rate, double frequency)
	{
		return std::tan(std::numbers::pi * std::min(frequency, sample_rate * HIGHEST_CUTOFF_RATIO) / sample_rate);
	}

	double shelf_amplitude(float gain_db)
	{
		return std::pow(10.0, gain_db / 40.0);
	}

	SvfCoefficients solved(double g, double k, double input_mix, double band_mix, double low_mix)
	{
		const auto a1 = 1.0 / (1.0 + g * (g + k));
		const auto a2 = g * a1;
		const auto a3 = g * a2;
		return {(float) g, (float) k, (float) a1, (float) a2, (float) a3, (float) input_mix, (float) band_mix, (float) low_mix};
	}
}

void OnePole::set_cutoff(float frequency, double sample_rate)
{
	const auto g = warped(sample_rate, frequency);
	gain = (float) (g / (1.0 + g));
}

float OnePole::lowpass(float input)
{
	const auto step = (input - state) * gain;
	const auto output = step + state;
	state = output + step;
	return output;
}

SvfCoefficients SvfCoefficients::make(FilterShape shape, double sample_rate, float frequency, float q, float gain_db)
{
	const auto g = warped(sample_rate, frequency);
	const auto k = 1.0 / std::max(q, LOWEST_Q);
	const auto amplitude = shelf_amplitude(gain_db);

	switch (shape) {
		case FilterShape::bell: {
			const auto bell_k = k / amplitude;
			return solved(g, bell_k, 1.0, bell_k * (amplitude * amplitude - 1.0), 0.0);
		}
		case FilterShape::low_shelf:
			return solved(g / std::sqrt(amplitude), k, 1.0, k * (amplitude - 1.0), amplitude * amplitude - 1.0);
		case FilterShape::high_shelf:
			return solved(g * std::sqrt(amplitude), k, amplitude * amplitude, k * (1.0 - amplitude) * amplitude, 1.0 - amplitude * amplitude);
		case FilterShape::lowpass:
			return solved(g, k, 0.0, 0.0, 1.0);
		case FilterShape::highpass:
			break;
	}

	return solved(g, k, 1.0, -k, -1.0);
}

float SvfCoefficients::magnitude_db(double sample_rate, float frequency) const
{
	const auto omega = warped(sample_rate, frequency) / g;
	const std::complex<double> s{0.0, omega};
	const auto denominator = s * s + (double) k * s + 1.0;
	const auto response = (double) input_mix + ((double) band_mix * s + (double) low_mix) / denominator;
	return 20.0f * std::log10(std::max((float) std::abs(response), LOWEST_MAGNITUDE));
}

float Svf::process(float input)
{
	const auto& c = coefficients;
	const auto difference = input - low_state;
	const auto band = c.a1 * band_state + c.a2 * difference;
	const auto low = low_state + c.a2 * band_state + c.a3 * difference;
	band_state = 2.0f * band - band_state;
	low_state = 2.0f * low - low_state;
	return c.input_mix * input + c.band_mix * band + c.low_mix * low;
}

float butterworth_q(int order, int section)
{
	const auto angle = std::numbers::pi * (2.0 * section + 1.0) / (2.0 * order);
	return (float) (1.0 / (2.0 * std::cos(angle)));
}

float octave_bandwidth_q(float octaves)
{
	const auto ratio = std::pow(2.0, (double) octaves);
	return (float) (std::sqrt(ratio) / (ratio - 1.0));
}

float audio_taper(float position)
{
	const auto clamped = std::clamp(position, 0.0f, 1.0f);
	return (std::pow(10.0f, TAPER_DECADES * clamped) - 1.0f) / (std::pow(10.0f, TAPER_DECADES) - 1.0f);
}
