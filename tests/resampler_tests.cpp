#include "engine/resampler.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <vector>

namespace
{
	constexpr double PI = 3.14159265358979323846;

	double sine(double frequency, double rate, double position)
	{
		return std::sin(2.0 * PI * frequency * position / rate);
	}

	double worst_error(double source_rate, double target_rate, double frequency)
	{
		const auto step = source_rate / target_rate;
		const ResamplingKernel kernel(step);
		constexpr int SOURCE_LENGTH = 8192;
		constexpr int OUTPUT_LENGTH = 4096;
		constexpr double START = 64.0;

		std::vector<float> source(SOURCE_LENGTH);

		for (int index = 0; index < SOURCE_LENGTH; ++index)
			source[(size_t) index] = (float) sine(frequency, source_rate, index);

		std::vector<float> output(OUTPUT_LENGTH);
		kernel.render(source.data(), START, OUTPUT_LENGTH, output.data());

		double error = 0.0;

		for (int index = 0; index < OUTPUT_LENGTH; ++index)
			error = std::max(error, std::abs(output[(size_t) index] - sine(frequency, source_rate, START + index * step)));

		return error;
	}
}

TEST_CASE("resampling keeps a tone clean in both directions")
{
	CHECK(worst_error(44100.0, 48000.0, 1000.0) < 0.001);
	CHECK(worst_error(48000.0, 44100.0, 1000.0) < 0.001);
	CHECK(worst_error(44100.0, 48000.0, 10000.0) < 0.01);
}

TEST_CASE("resampling a constant signal keeps its level")
{
	const ResamplingKernel kernel(48000.0 / 44100.0);
	const std::vector<float> source(256, 0.5f);
	std::vector<float> output(64);
	kernel.render(source.data(), 32.3, 64, output.data());

	for (const auto value : output)
		CHECK(std::abs(value - 0.5f) < 1.0e-4f);
}
