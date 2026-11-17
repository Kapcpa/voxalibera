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

TEST_CASE("a block that reaches the loop end stops there and the next one starts at the loop start")
{
	Transport transport;
	transport.set_loop({true, 1000, 2000});
	transport.locate(1900);
	transport.play();

	const auto first = transport.advance(256);
	CHECK(first.start == 1900);
	CHECK(first.length == 100);

	const auto second = transport.advance(156);
	CHECK(second.start == 1000);
	CHECK(second.length == 156);
}

TEST_CASE("playing from after the loop end does not wrap")
{
	Transport transport;
	transport.set_loop({true, 1000, 2000});
	transport.locate(2100);
	transport.play();

	const auto block = transport.advance(256);
	CHECK(block.length == 256);
	CHECK(transport.position() == 2356);
}

TEST_CASE("recording keeps going after the loop wraps")
{
	Transport transport;
	transport.set_loop({true, 1000, 2000});
	transport.locate(1500);
	transport.record();

	transport.advance(500);
	const auto wrapped = transport.advance(256);
	CHECK(wrapped.start == 1000);
	CHECK(wrapped.record_from <= wrapped.start);
	CHECK_FALSE(wrapped.is_counting_in());
}
