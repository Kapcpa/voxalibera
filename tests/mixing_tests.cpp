#include "core/project/model.h"
#include "engine/levels.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>

namespace
{
	bool near(float value, float expected)
	{
		return std::abs(value - expected) < 0.001f;
	}

	Track track(bool mute, bool solo)
	{
		Track result;
		result.mute = mute;
		result.solo = solo;
		return result;
	}
}

TEST_CASE("faders convert decibels to gain, with the bottom as silence")
{
	CHECK(near(fader_gain(0.0f), 1.0f));
	CHECK(near(fader_gain(-6.0f), 0.501f));
	CHECK(near(fader_gain(SILENT_DB), 0.0f));
}

TEST_CASE("mono pan keeps the same power across the field")
{
	const auto centre = mono_pan_gains(0.0f);
	const auto left = mono_pan_gains(-1.0f);

	CHECK(near(centre.left, 0.707f));
	CHECK(near(centre.right, 0.707f));
	CHECK(near(left.left, 1.0f));
	CHECK(near(left.right, 0.0f));
}

TEST_CASE("stereo balance only turns the opposite side down")
{
	const auto centre = stereo_balance_gains(0.0f);
	const auto half_right = stereo_balance_gains(0.5f);

	CHECK(near(centre.left, 1.0f));
	CHECK(near(centre.right, 1.0f));
	CHECK(near(half_right.left, 0.5f));
	CHECK(near(half_right.right, 1.0f));
}

TEST_CASE("solo silences every track that isn't soloed, mute always silences")
{
	const std::vector tracks{track(false, true), track(false, false), track(true, true)};
	const auto solo_active = any_solo(tracks);

	CHECK(is_audible(tracks[0], solo_active));
	CHECK_FALSE(is_audible(tracks[1], solo_active));
	CHECK_FALSE(is_audible(tracks[2], solo_active));
	CHECK(is_audible(track(false, false), false));
}
