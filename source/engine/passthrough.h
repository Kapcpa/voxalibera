#pragma once

#include <atomic>

enum class InputRouting
{
	stereo,
	left_to_both,
	right_to_both
};

class Passthrough
{
public:
	void add_to(const float* const* inputs, int input_count, float* const* outputs, int output_count, int sample_count) const;

	std::atomic<bool> monitoring{false};
	std::atomic<InputRouting> routing{InputRouting::left_to_both};

private:
	int source_for(int output_channel, int input_count) const;
};
