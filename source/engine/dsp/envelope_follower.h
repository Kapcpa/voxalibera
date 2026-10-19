#pragma once

class EnvelopeFollower
{
public:
	void set_times(float attack_seconds, float release_seconds, double sample_rate);
	void reset() { level = 0.0f; }
	float process(float input);

private:
	float attack = 0.0f;
	float release = 0.0f;
	float level = 0.0f;
};
