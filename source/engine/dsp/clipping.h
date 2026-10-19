#pragma once

#include <juce_core/juce_core.h>

inline constexpr float VOLTS_AT_FULL_SCALE = 1.0f;

enum class DiodeType
{
	silicon,
	germanium,
	led,
	asymmetric
};

enum class DiodePlacement
{
	feedback,
	to_ground
};

inline const juce::StringArray DIODE_CHOICES{"Silicon", "Germanium", "LED", "Asymmetric"};

struct Diodes
{
	float positive_volts = 0.6f;
	float negative_volts = 0.6f;
	float knee = 2.5f;
};

Diodes diodes_of(DiodeType type, DiodePlacement placement, int in_series = 1);
DiodeType diode_type_at(float choice);
float diode_clip(float volts, const Diodes& diodes);
float rail_clip(float volts, float rail_volts);
float soft_dead_zone(float volts, float width_volts);
