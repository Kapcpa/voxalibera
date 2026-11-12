#pragma once

#include <vector>

class ResamplingKernel
{
public:
	static constexpr int REALTIME_TAPS = 32;
	static constexpr int EXPORT_TAPS = 128;
	static constexpr int MAX_TAPS = EXPORT_TAPS;
	static constexpr int PHASES = 256;
	static constexpr double MAX_STEP = 4.0;

	explicit ResamplingKernel(double step, int taps = REALTIME_TAPS);

	double step() const { return source_step; }
	int taps() const { return tap_count; }
	int half_taps() const { return tap_count / 2; }
	void render(const float* source, double position, int count, float* destination) const;

private:
	double source_step;
	int tap_count;
	std::vector<float> table;
};
