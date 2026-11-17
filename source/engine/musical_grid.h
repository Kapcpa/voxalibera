#pragma once

#include "engine/musical_time.h"

#include <juce_core/juce_core.h>

#include <cstdint>
#include <optional>
#include <vector>

enum class GridDivision
{
	bar,
	half,
	quarter,
	eighth,
	sixteenth,
	thirty_second,
};

struct Grid
{
	static constexpr int MIN_DENSITY = -2;
	static constexpr int MAX_DENSITY = 2;

	bool snap = true;
	bool adaptive = true;
	int density = 0;
	GridDivision division = GridDivision::sixteenth;
	bool triplets = false;

	bool operator==(const Grid&) const = default;
};

struct GridLine
{
	double beat = 0.0;
	bool is_bar = false;
};

std::int64_t ticks_per_bar(const TimeSignature& signature);
std::int64_t grid_step_ticks(const Grid& grid, const TimeSignature& signature, double pixels_per_beat);
double snap_to_grid(double beat, std::int64_t step_ticks, const TimeSignature& signature);
std::vector<GridLine> grid_lines(double first_beat, double last_beat, std::int64_t step_ticks, const TimeSignature& signature);
Grid finer_grid(Grid grid);
Grid coarser_grid(Grid grid);
juce::String describe_grid_step(std::int64_t step_ticks, const TimeSignature& signature);
juce::String grid_division_name(GridDivision division);
std::optional<GridDivision> parse_grid_division(const juce::String& name);
