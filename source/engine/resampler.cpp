#include "engine/resampler.h"

#include <juce_core/juce_core.h>

#include <cmath>

namespace
{
	constexpr double KAISER_BETA = 8.0;
	constexpr double CUTOFF_GUARD = 0.95;
	constexpr int BESSEL_TERMS = 24;

	double bessel_i0(double x)
	{
		double sum = 1.0;
		double term = 1.0;

		for (int k = 1; k < BESSEL_TERMS; ++k) {
			term *= (x / (2.0 * k)) * (x / (2.0 * k));
			sum += term;
		}

		return sum;
	}

	double kaiser(double x)
	{
		if (std::abs(x) >= 1.0)
			return 0.0;

		return bessel_i0(KAISER_BETA * std::sqrt(1.0 - x * x)) / bessel_i0(KAISER_BETA);
	}

	double sinc(double x)
	{
		if (std::abs(x) < 1.0e-9)
			return 1.0;

		const auto angle = juce::MathConstants<double>::pi * x;
		return std::sin(angle) / angle;
	}
}

ResamplingKernel::ResamplingKernel(double step) : source_step(step), table((size_t) ((PHASES + 1) * TAPS))
{
	const auto cutoff = std::min(1.0, 1.0 / step) * CUTOFF_GUARD;

	for (int phase = 0; phase <= PHASES; ++phase) {
		const auto fraction = (double) phase / PHASES;
		auto* row = table.data() + phase * TAPS;
		double sum = 0.0;

		for (int tap = 0; tap < TAPS; ++tap) {
			const auto distance = (double) (tap - HALF_TAPS + 1) - fraction;
			const auto weight = cutoff * sinc(cutoff * distance) * kaiser(distance / HALF_TAPS);
			row[tap] = (float) weight;
			sum += weight;
		}

		for (int tap = 0; tap < TAPS; ++tap)
			row[tap] = (float) (row[tap] / sum);
	}
}

void ResamplingKernel::render(const float* source, double position, int count, float* destination) const
{
	for (int sample = 0; sample < count; ++sample) {
		const auto at = position + sample * source_step;
		const auto whole = std::floor(at);
		const auto phase = (at - whole) * PHASES;
		const auto row_index = std::min(PHASES - 1, (int) phase);
		const auto blend = (float) (phase - row_index);
		const auto* first_row = table.data() + row_index * TAPS;
		const auto* second_row = first_row + TAPS;
		const auto* input = source + (std::ptrdiff_t) whole - HALF_TAPS + 1;
		float value = 0.0f;

		for (int tap = 0; tap < TAPS; ++tap)
			value += input[tap] * (first_row[tap] + blend * (second_row[tap] - first_row[tap]));

		destination[sample] = value;
	}
}
