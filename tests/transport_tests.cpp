#include "engine/transport.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("a count-in starts early and records from the original position")
{
	Transport transport;
	transport.locate(1000);
	transport.record(500);

	const auto first = transport.advance(400);
	CHECK(first.start == 500);
	CHECK(first.record_from == 1000);
	CHECK(first.is_counting_in());

	const auto second = transport.advance(400);
	CHECK(second.start == 900);
	CHECK(second.is_counting_in());

	const auto third = transport.advance(400);
	CHECK(third.start == 1300);
	CHECK_FALSE(third.is_counting_in());
	CHECK(third.recording);
}

TEST_CASE("recording while playing punches in without a count-in")
{
	Transport transport;
	transport.play();
	transport.advance(256);
	transport.record(500);

	const auto block = transport.advance(256);
	CHECK(block.start == 256);
	CHECK(block.record_from == 256);
	CHECK_FALSE(block.is_counting_in());
}
