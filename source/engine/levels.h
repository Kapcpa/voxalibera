#pragma once

#include <array>
#include <atomic>

constexpr float SILENT_DB = -60.0f;

struct StereoGains
{
	float left = 1.0f;
	float right = 1.0f;
};

float fader_gain(float decibels);
float fade_gain(float progress);
float clip_fade_gain(std::int64_t position, std::int64_t length, std::int64_t fade_in, std::int64_t fade_out);
StereoGains mono_pan_gains(float pan);
StereoGains stereo_balance_gains(float pan);

class LevelMeter
{
public:
	static constexpr int CHANNELS = 2;

	void measure(const float* const* channels, int channel_count, int sample_count);
	float take_peak(int channel);

private:
	std::array<std::atomic<float>, CHANNELS> peaks{};
};
