#pragma once

#include <juce_core/juce_core.h>

#include <array>
#include <cstdint>

struct TrackColour
{
	const char* name;
	std::uint32_t argb;
};

inline constexpr std::array<TrackColour, 8> TRACK_COLOURS{{
	{"Orange", 0xffd9813b},
	{"Blue", 0xff5b8def},
	{"Green", 0xff4fb37a},
	{"Pink", 0xffc75c9a},
	{"Yellow", 0xffd4b84a},
	{"Teal", 0xff58b8c4},
	{"Purple", 0xff9b7be0},
	{"Red", 0xffe0675c},
}};
