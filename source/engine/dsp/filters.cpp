#include "engine/dsp/filters.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace
{
	constexpr double HIGHEST_CUTOFF_RATIO = 0.49;
	constexpr float SHELF_SLOPE_Q = 0.7071f;
	constexpr float TAPER_DECADES = 2.0f;

	struct Bilinear
	{
		double cosine;
		double alpha;
	};

	Bilinear bilinear(double sample_rate, float frequency, float q)
	{
		const auto clamped = std::min((double) frequency, sample_rate * HIGHEST_CUTOFF_RATIO);
		const auto omega = 2.0 * std::numbers::pi * clamped / sample_rate;
		return {std::cos(omega), std::sin(omega) / (2.0 * q)};
	}

	BiquadCoefficients normalised(double b0, double b1, double b2, double a0, double a1, double a2)
	{
		return {(float) (b0 / a0), (float) (b1 / a0), (float) (b2 / a0), (float) (a1 / a0), (float) (a2 / a0)};
	}

	double shelf_amplitude(float gain_db)
	{
		return std::pow(10.0, gain_db / 40.0);
	}
}

void OnePole::set_cutoff(float frequency, double sample_rate)
{
	const auto clamped = std::min((double) frequency, sample_rate * HIGHEST_CUTOFF_RATIO);
	const auto warped = std::tan(std::numbers::pi * clamped / sample_rate);
	gain = (float) (warped / (1.0 + warped));
}

float OnePole::lowpass(float input)
{
	const auto step = (input - state) * gain;
	const auto output = step + state;
	state = output + step;
	return output;
}

BiquadCoefficients BiquadCoefficients::lowpass(double sample_rate, float frequency, float q)
{
	const auto [cosine, alpha] = bilinear(sample_rate, frequency, q);
	return normalised((1.0 - cosine) / 2.0, 1.0 - cosine, (1.0 - cosine) / 2.0, 1.0 + alpha, -2.0 * cosine, 1.0 - alpha);
}

BiquadCoefficients BiquadCoefficients::highpass(double sample_rate, float frequency, float q)
{
	const auto [cosine, alpha] = bilinear(sample_rate, frequency, q);
	return normalised((1.0 + cosine) / 2.0, -(1.0 + cosine), (1.0 + cosine) / 2.0, 1.0 + alpha, -2.0 * cosine, 1.0 - alpha);
}

BiquadCoefficients BiquadCoefficients::peak(double sample_rate, float frequency, float q, float gain_db)
{
	const auto [cosine, alpha] = bilinear(sample_rate, frequency, q);
	const auto amplitude = shelf_amplitude(gain_db);
	return normalised(1.0 + alpha * amplitude, -2.0 * cosine, 1.0 - alpha * amplitude, 1.0 + alpha / amplitude, -2.0 * cosine, 1.0 - alpha / amplitude);
}

BiquadCoefficients BiquadCoefficients::low_shelf(double sample_rate, float frequency, float gain_db)
{
	const auto [cosine, alpha] = bilinear(sample_rate, frequency, SHELF_SLOPE_Q);
	const auto amplitude = shelf_amplitude(gain_db);
	const auto root = 2.0 * std::sqrt(amplitude) * alpha;
	return normalised(amplitude * ((amplitude + 1.0) - (amplitude - 1.0) * cosine + root), 2.0 * amplitude * ((amplitude - 1.0) - (amplitude + 1.0) * cosine), amplitude * ((amplitude + 1.0) - (amplitude - 1.0) * cosine - root), (amplitude + 1.0) + (amplitude - 1.0) * cosine + root, -2.0 * ((amplitude - 1.0) + (amplitude + 1.0) * cosine), (amplitude + 1.0) + (amplitude - 1.0) * cosine - root);
}

BiquadCoefficients BiquadCoefficients::high_shelf(double sample_rate, float frequency, float gain_db)
{
	const auto [cosine, alpha] = bilinear(sample_rate, frequency, SHELF_SLOPE_Q);
	const auto amplitude = shelf_amplitude(gain_db);
	const auto root = 2.0 * std::sqrt(amplitude) * alpha;
	return normalised(amplitude * ((amplitude + 1.0) + (amplitude - 1.0) * cosine + root), -2.0 * amplitude * ((amplitude - 1.0) + (amplitude + 1.0) * cosine), amplitude * ((amplitude + 1.0) + (amplitude - 1.0) * cosine - root), (amplitude + 1.0) - (amplitude - 1.0) * cosine + root, 2.0 * ((amplitude - 1.0) - (amplitude + 1.0) * cosine), (amplitude + 1.0) - (amplitude - 1.0) * cosine - root);
}

float Biquad::process(float input)
{
	const auto output = coefficients.b0 * input + first;
	first = coefficients.b1 * input - coefficients.a1 * output + second;
	second = coefficients.b2 * input - coefficients.a2 * output;
	return output;
}

float audio_taper(float position)
{
	const auto clamped = std::clamp(position, 0.0f, 1.0f);
	return (std::pow(10.0f, TAPER_DECADES * clamped) - 1.0f) / (std::pow(10.0f, TAPER_DECADES) - 1.0f);
}
