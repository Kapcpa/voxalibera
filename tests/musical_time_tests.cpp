#include "engine/musical_time.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>

namespace
{
	constexpr double SAMPLE_RATE = 48000.0;
	const Tempo FOUR_FOUR_AT_120{120.0, {4, 4}};
	const Tempo SIX_EIGHT_AT_120{120.0, {6, 8}};
}

TEST_CASE("a quarter note at 120 bpm lasts half a second")
{
	CHECK(std::lround(samples_per_beat(FOUR_FOUR_AT_120, SAMPLE_RATE)) == 24000);
	CHECK(std::lround(samples_per_beat(SIX_EIGHT_AT_120, SAMPLE_RATE)) == 12000);
}

TEST_CASE("samples convert to bars, beats and ticks")
{
	CHECK(to_musical_position(0, FOUR_FOUR_AT_120, SAMPLE_RATE) == MusicalPosition{1, 1, 0});
	CHECK(to_musical_position(12000, FOUR_FOUR_AT_120, SAMPLE_RATE) == MusicalPosition{1, 1, 480});
	CHECK(to_musical_position(24000, FOUR_FOUR_AT_120, SAMPLE_RATE) == MusicalPosition{1, 2, 0});
	CHECK(to_musical_position(96000, FOUR_FOUR_AT_120, SAMPLE_RATE) == MusicalPosition{2, 1, 0});
	CHECK(to_musical_position(72000, SIX_EIGHT_AT_120, SAMPLE_RATE) == MusicalPosition{2, 1, 0});
}

TEST_CASE("musical positions convert back to the same samples")
{
	for (const auto sample : {0, 12000, 24000, 96000, 1234567})
		CHECK(to_sample(to_musical_position(sample, FOUR_FOUR_AT_120, SAMPLE_RATE), FOUR_FOUR_AT_120, SAMPLE_RATE) == sample - sample % 25);
}

TEST_CASE("positions are shown as bars, beats and ticks and as a clock")
{
	CHECK(format_musical_position({3, 2, 7}) == "3.2.007");
	CHECK(format_clock(48000 * 65 + 24000, SAMPLE_RATE) == "01:05.500");
}

TEST_CASE("time signatures are read from text and rejected when they make no sense")
{
	CHECK(parse_time_signature("7/8") == TimeSignature{7, 8});
	CHECK(parse_time_signature(" 11 / 16 ") == TimeSignature{11, 16});
	CHECK_FALSE(parse_time_signature("0/4"));
	CHECK_FALSE(parse_time_signature("4/3"));
	CHECK_FALSE(parse_time_signature("abc"));
	CHECK_FALSE(parse_time_signature("4/4/4"));
	CHECK(format_time_signature({12, 8}) == "12/8");
}
