#include "engine/dsp/clipping.h"

#include <cmath>

namespace
{
	constexpr float SILICON_VOLTS = 0.6f;
	constexpr float GERMANIUM_VOLTS = 0.3f;
	constexpr float LED_VOLTS = 1.7f;
	constexpr float SILICON_KNEE = 2.5f;
	constexpr float GERMANIUM_KNEE = 1.5f;
	constexpr float LED_KNEE = 3.0f;
	constexpr float TO_GROUND_HARDNESS = 1.6f;
	constexpr float RAIL_KNEE = 6.0f;
	constexpr float SMALLEST_WIDTH = 1.0e-4f;

	Diodes forward_voltages(DiodeType type)
	{
		switch (type) {
			case DiodeType::germanium:
				return {GERMANIUM_VOLTS, GERMANIUM_VOLTS, GERMANIUM_KNEE};
			case DiodeType::led:
				return {LED_VOLTS, LED_VOLTS, LED_KNEE};
			case DiodeType::asymmetric:
				return {SILICON_VOLTS, 2.0f * SILICON_VOLTS, SILICON_KNEE};
			case DiodeType::silicon:
				break;
		}

		return {SILICON_VOLTS, SILICON_VOLTS, SILICON_KNEE};
	}

	float knee_clip(float volts, float limit, float knee)
	{
		return volts / std::pow(1.0f + std::pow(std::abs(volts) / limit, knee), 1.0f / knee);
	}
}

Diodes diodes_of(DiodeType type, DiodePlacement placement, int in_series)
{
	auto diodes = forward_voltages(type);
	diodes.positive_volts *= (float) in_series;
	diodes.negative_volts *= (float) in_series;

	if (placement == DiodePlacement::to_ground)
		diodes.knee *= TO_GROUND_HARDNESS;

	return diodes;
}

DiodeType diode_type_at(float choice)
{
	return (DiodeType) juce::jlimit(0, DIODE_CHOICES.size() - 1, juce::roundToInt(choice));
}

float diode_clip(float volts, const Diodes& diodes)
{
	return knee_clip(volts, volts >= 0.0f ? diodes.positive_volts : diodes.negative_volts, diodes.knee);
}

float rail_clip(float volts, float rail_volts)
{
	return knee_clip(volts, rail_volts, RAIL_KNEE);
}

float soft_dead_zone(float volts, float width_volts)
{
	if (width_volts < SMALLEST_WIDTH)
		return volts;

	return volts - width_volts * std::tanh(volts / width_volts);
}
