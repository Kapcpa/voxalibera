#pragma once

#include <array>
#include <atomic>

class LevelMeter
{
public:
	static constexpr int CHANNELS = 2;

	void measure(const float* const* channels, int channel_count, int sample_count);
	float take_peak(int channel);

private:
	std::array<std::atomic<float>, CHANNELS> peaks{};
};
