#pragma once

#include <compare>

template <typename Tag>
struct Id
{
	int value = 0;

	bool is_valid() const { return value > 0; }
	auto operator<=>(const Id&) const = default;
};

using TrackId = Id<struct TrackTag>;
using ClipId = Id<struct ClipTag>;
