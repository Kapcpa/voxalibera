#include "engine/dsp/filters.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>
#include <numbers>
#include <vector>

namespace
{
	constexpr double SAMPLE_RATE = 44100.0;
	constexpr int SETTLE_SAMPLES = 22050;
	constexpr int MEASURE_SAMPLES = 22050;
	constexpr float TOLERANCE_DB = 0.1f;
	constexpr float PROBES[]{50.0f, 200.0f, 1000.0f, 3000.0f, 8000.0f, 15000.0f};

	float measured_db(const SvfCoefficients& coefficients, float frequency)
	{
		Svf filter;
		filter.set(coefficients);
		double input_energy = 0.0;
		double output_energy = 0.0;

		for (int index = 0; index < SETTLE_SAMPLES + MEASURE_SAMPLES; ++index) {
			const auto input = (float) std::sin(2.0 * std::numbers::pi * frequency * index / SAMPLE_RATE);
			const auto output = filter.process(input);

			if (index < SETTLE_SAMPLES)
				continue;

			input_energy += (double) input * input;
			output_energy += (double) output * output;
		}

		return (float) (10.0 * std::log10(output_energy / input_energy));
	}
}

TEST_CASE("the drawn response of a filter matches what it does to sound")
{
	const std::vector<SvfCoefficients> filters{
		SvfCoefficients::make(FilterShape::bell, SAMPLE_RATE, 1000.0f, octave_bandwidth_q(1.0f), 9.0f),
		SvfCoefficients::make(FilterShape::bell, SAMPLE_RATE, 6000.0f, 4.0f, -12.0f),
		SvfCoefficients::make(FilterShape::low_shelf, SAMPLE_RATE, 200.0f, 0.7071f, -6.0f),
		SvfCoefficients::make(FilterShape::high_shelf, SAMPLE_RATE, 5000.0f, 0.7071f, 6.0f),
		SvfCoefficients::make(FilterShape::lowpass, SAMPLE_RATE, 2000.0f, butterworth_q(4, 1)),
		SvfCoefficients::make(FilterShape::highpass, SAMPLE_RATE, 100.0f, butterworth_q(2, 0)),
	};

	for (size_t filter = 0; filter < filters.size(); ++filter)
		for (const auto probe : PROBES) {
			INFO("filter " << filter + 1 << " at " << probe << " Hz");
			CHECK_THAT(measured_db(filters[filter], probe), Catch::Matchers::WithinAbs(filters[filter].magnitude_db(SAMPLE_RATE, probe), TOLERANCE_DB));
		}
}

TEST_CASE("a flat bell leaves the sound untouched")
{
	const auto flat = SvfCoefficients::make(FilterShape::bell, SAMPLE_RATE, 1000.0f, 1.41f, 0.0f);

	for (const auto probe : PROBES)
		CHECK_THAT(measured_db(flat, probe), Catch::Matchers::WithinAbs(0.0, 0.001));
}
