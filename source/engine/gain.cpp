#include "engine/gain.h"

#include <algorithm>
#include <cmath>
#include <numbers>

float fader_gain(float decibels)
{
	return decibels <= SILENT_DB ? 0.0f : std::pow(10.0f, decibels / 20.0f);
}

StereoGains mono_pan_gains(float pan)
{
	const auto angle = (std::clamp(pan, -1.0f, 1.0f) + 1.0f) * std::numbers::pi_v<float> / 4.0f;
	return {std::cos(angle), std::sin(angle)};
}

StereoGains stereo_balance_gains(float pan)
{
	const auto clamped = std::clamp(pan, -1.0f, 1.0f);
	return {std::min(1.0f, 1.0f - clamped), std::min(1.0f, 1.0f + clamped)};
}
