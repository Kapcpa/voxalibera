#include "engine/musical_grid.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

namespace
{
	constexpr double ADAPTIVE_SPACING_PIXELS = 16.0;
	constexpr int FINEST_BEAT_HALVINGS = 4;
	constexpr std::int64_t MAX_ADAPTIVE_BARS = 1024;
	constexpr std::int64_t TRIPLET_NUMERATOR = 2;
	constexpr std::int64_t TRIPLET_DENOMINATOR = 3;

	const std::array<std::pair<GridDivision, const char*>, 6> DIVISION_NAMES{{
		{GridDivision::bar, "bar"},
		{GridDivision::half, "1/2"},
		{GridDivision::quarter, "1/4"},
		{GridDivision::eighth, "1/8"},
		{GridDivision::sixteenth, "1/16"},
		{GridDivision::thirty_second, "1/32"},
	}};

	std::int64_t ticks_per_whole_note(const TimeSignature& signature)
	{
		return (std::int64_t) TICKS_PER_BEAT * signature.beat_unit;
	}

	std::int64_t note_denominator(GridDivision division)
	{
		return std::int64_t{1} << (int) division;
	}

	std::int64_t as_triplet(std::int64_t step)
	{
		return step * TRIPLET_NUMERATOR / TRIPLET_DENOMINATOR;
	}

	std::int64_t whole_bars_when_longer(std::int64_t step, std::int64_t bar)
	{
		return step > bar ? step / bar * bar : step;
	}

	std::int64_t fixed_step(const Grid& grid, const TimeSignature& signature)
	{
		const auto bar = ticks_per_bar(signature);

		if (grid.division == GridDivision::bar)
			return bar;

		const auto note = ticks_per_whole_note(signature) / note_denominator(grid.division);
		return whole_bars_when_longer(grid.triplets ? as_triplet(note) : note, bar);
	}

	std::int64_t adaptive_step(const Grid& grid, const TimeSignature& signature, double pixels_per_beat)
	{
		const auto min_pixels = ADAPTIVE_SPACING_PIXELS / std::pow(2.0, grid.density);
		const auto pixels_per_tick = pixels_per_beat / TICKS_PER_BEAT;
		const auto beat = grid.triplets ? as_triplet(TICKS_PER_BEAT) : (std::int64_t) TICKS_PER_BEAT;

		for (int halvings = FINEST_BEAT_HALVINGS; halvings >= 0; --halvings)
			if (const auto step = beat >> halvings; (double) step * pixels_per_tick >= min_pixels)
				return step;

		const auto bar = ticks_per_bar(signature);
		auto step = bar;

		while ((double) step * pixels_per_tick < min_pixels && step < bar * MAX_ADAPTIVE_BARS)
			step *= 2;

		return step;
	}
}

std::int64_t ticks_per_bar(const TimeSignature& signature)
{
	return (std::int64_t) TICKS_PER_BEAT * signature.beats_per_bar;
}

std::int64_t grid_step_ticks(const Grid& grid, const TimeSignature& signature, double pixels_per_beat)
{
	if (!grid.adaptive)
		return fixed_step(grid, signature);

	if (pixels_per_beat <= 0.0)
		return ticks_per_bar(signature);

	return adaptive_step(grid, signature, pixels_per_beat);
}

double snap_to_grid(double beat, std::int64_t step_ticks, const TimeSignature& signature)
{
	if (step_ticks <= 0)
		return beat;

	const auto bar = (double) ticks_per_bar(signature);
	const auto step = (double) step_ticks;
	const auto tick = beat * TICKS_PER_BEAT;

	if (step >= bar)
		return std::round(tick / step) * step / TICKS_PER_BEAT;

	const auto bar_start = std::floor(tick / bar) * bar;
	const auto within = tick - bar_start;
	const auto nearest_step = std::min(std::round(within / step) * step, bar);
	const auto snapped = bar - within < std::abs(nearest_step - within) ? bar : nearest_step;
	return (bar_start + snapped) / TICKS_PER_BEAT;
}

std::vector<GridLine> grid_lines(double first_beat, double last_beat, std::int64_t step_ticks, const TimeSignature& signature)
{
	std::vector<GridLine> lines;

	if (step_ticks <= 0)
		return lines;

	const auto period = std::max(step_ticks, ticks_per_bar(signature));
	const auto first_tick = (std::int64_t) std::floor(first_beat * TICKS_PER_BEAT);
	const auto last_tick = (std::int64_t) std::ceil(last_beat * TICKS_PER_BEAT);
	const auto first_period = (std::int64_t) std::floor((double) first_tick / (double) period) * period;

	for (auto period_start = first_period; period_start <= last_tick; period_start += period)
		for (std::int64_t offset = 0; offset < period; offset += step_ticks)
			if (const auto tick = period_start + offset; tick >= first_tick && tick <= last_tick)
				lines.push_back({(double) tick / TICKS_PER_BEAT, offset == 0});

	return lines;
}

Grid finer_grid(Grid grid)
{
	if (grid.adaptive)
		grid.density = std::min(grid.density + 1, Grid::MAX_DENSITY);
	else
		grid.division = (GridDivision) std::min((int) grid.division + 1, (int) GridDivision::thirty_second);

	return grid;
}

Grid coarser_grid(Grid grid)
{
	if (grid.adaptive)
		grid.density = std::max(grid.density - 1, Grid::MIN_DENSITY);
	else
		grid.division = (GridDivision) std::max((int) grid.division - 1, (int) GridDivision::bar);

	return grid;
}

juce::String describe_grid_step(std::int64_t step_ticks, const TimeSignature& signature)
{
	const auto bar = ticks_per_bar(signature);
	const auto whole = ticks_per_whole_note(signature);

	if (step_ticks <= 0)
		return {};

	if (step_ticks % bar == 0)
		return step_ticks == bar ? juce::String("1 bar") : juce::String(step_ticks / bar) + " bars";

	if (whole % step_ticks == 0 && juce::isPowerOfTwo(whole / step_ticks))
		return "1/" + juce::String(whole / step_ticks);

	if (whole * TRIPLET_NUMERATOR % (step_ticks * TRIPLET_DENOMINATOR) == 0)
		return "1/" + juce::String(whole * TRIPLET_NUMERATOR / (step_ticks * TRIPLET_DENOMINATOR)) + "T";

	return juce::String(step_ticks) + " ticks";
}

juce::String grid_division_name(GridDivision division)
{
	for (const auto& [value, name] : DIVISION_NAMES)
		if (value == division)
			return name;

	return {};
}

std::optional<GridDivision> parse_grid_division(const juce::String& name)
{
	for (const auto& [value, division_name] : DIVISION_NAMES)
		if (name == division_name)
			return value;

	return std::nullopt;
}
