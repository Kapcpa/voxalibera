#include "engine/musical_grid.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

namespace
{
	const TimeSignature FOUR_FOUR{4, 4};
	const TimeSignature SIX_EIGHT{6, 8};
	const TimeSignature SEVEN_EIGHT{7, 8};

	Grid fixed(GridDivision division, bool triplets = false)
	{
		Grid grid;
		grid.adaptive = false;
		grid.division = division;
		grid.triplets = triplets;
		return grid;
	}
}

TEST_CASE("fixed grid steps are note values in ticks")
{
	CHECK(grid_step_ticks(fixed(GridDivision::quarter), FOUR_FOUR, 24.0) == 960);
	CHECK(grid_step_ticks(fixed(GridDivision::sixteenth), FOUR_FOUR, 24.0) == 240);
	CHECK(grid_step_ticks(fixed(GridDivision::eighth, true), FOUR_FOUR, 24.0) == 320);
	CHECK(grid_step_ticks(fixed(GridDivision::eighth), SIX_EIGHT, 24.0) == 960);
	CHECK(grid_step_ticks(fixed(GridDivision::bar), SIX_EIGHT, 24.0) == 5760);
}

TEST_CASE("adaptive grid gets finer as the view zooms in")
{
	const Grid adaptive;

	CHECK(grid_step_ticks(adaptive, FOUR_FOUR, 2.0) > grid_step_ticks(adaptive, FOUR_FOUR, 24.0));
	CHECK(grid_step_ticks(adaptive, FOUR_FOUR, 24.0) > grid_step_ticks(adaptive, FOUR_FOUR, 400.0));
	CHECK(grid_step_ticks(adaptive, FOUR_FOUR, 2.0) % ticks_per_bar(FOUR_FOUR) == 0);
}

TEST_CASE("snapping lands on the nearest step and restarts every bar")
{
	CHECK(snap_to_grid(1.4, 960, FOUR_FOUR) == Catch::Approx(1.0));
	CHECK(snap_to_grid(1.6, 960, FOUR_FOUR) == Catch::Approx(2.0));
	CHECK(snap_to_grid(6.9, 1920, SEVEN_EIGHT) == Catch::Approx(7.0));
	CHECK(snap_to_grid(8.1, 1920, SEVEN_EIGHT) == Catch::Approx(9.0));
}

TEST_CASE("grid lines mark bars and steps")
{
	const auto lines = grid_lines(0.0, 4.0, 960, FOUR_FOUR);

	REQUIRE(lines.size() == 5);
	CHECK(lines[0].is_bar);
	CHECK(!lines[1].is_bar);
	CHECK(lines[4].is_bar);
}

TEST_CASE("grid steps are named as note values")
{
	CHECK(describe_grid_step(240, FOUR_FOUR) == "1/16");
	CHECK(describe_grid_step(640, FOUR_FOUR) == "1/4T");
	CHECK(describe_grid_step(7680, FOUR_FOUR) == "2 bars");
}
