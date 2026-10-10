#pragma once

#include <vector>

class ResamplingKernel
{
public:
	static constexpr int TAPS = 32;
	static constexpr int HALF_TAPS = TAPS / 2;
	static constexpr int PHASES = 256;
	static constexpr double MAX_STEP = 4.0;

	explicit ResamplingKernel(double step);

	double step() const { return source_step; }
	void render(const float* source, double position, int count, float* destination) const;

private:
	double source_step;
	std::vector<float> table;
};
