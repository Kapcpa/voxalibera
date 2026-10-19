#include "engine/dsp/envelope_follower.h"

#include <cmath>

namespace
{
	float smoothing(float seconds, double sample_rate)
	{
		return (float) std::exp(-1.0 / (seconds * sample_rate));
	}
}

void EnvelopeFollower::set_times(float attack_seconds, float release_seconds, double sample_rate)
{
	attack = smoothing(attack_seconds, sample_rate);
	release = smoothing(release_seconds, sample_rate);
}

float EnvelopeFollower::process(float input)
{
	const auto magnitude = std::abs(input);
	const auto coefficient = magnitude > level ? attack : release;
	level = magnitude + coefficient * (level - magnitude);
	return level;
}
