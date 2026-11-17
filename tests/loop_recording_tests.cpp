#include "engine/recording_session.h"

#include <catch2/catch_test_macros.hpp>

namespace
{
	const Transport::Loop LOOP{true, 1000, 2000};
	constexpr std::int64_t LATENCY = 50;
}

TEST_CASE("a take without a loop becomes one clip shifted by the latency")
{
	CHECK(last_full_pass(1500, 3000, LATENCY, {}) == RecordedRange{1500, LATENCY, 2950});
}

TEST_CASE("a take stopped before the loop wraps keeps everything")
{
	CHECK(last_full_pass(1500, 400, LATENCY, LOOP) == RecordedRange{1500, LATENCY, 350});
}

TEST_CASE("a take stopped during the second pass keeps the first one")
{
	CHECK(last_full_pass(1500, 1000, LATENCY, LOOP) == RecordedRange{1500, LATENCY, 500});
}

TEST_CASE("a looped take keeps the last full pass")
{
	const auto first_pass = 500;
	const auto written = LATENCY + first_pass + 2 * 1000 + 300;

	CHECK(last_full_pass(1500, written, LATENCY, LOOP) == RecordedRange{1000, LATENCY + first_pass + 1000, 1000});
}

TEST_CASE("the live take shows the pass being recorded")
{
	CHECK(current_pass(1500, 300, LOOP) == RecordedRange{1500, 0, 300});
	CHECK(current_pass(1500, 500 + 1000 + 250, LOOP) == RecordedRange{1000, 1500, 250});
}
