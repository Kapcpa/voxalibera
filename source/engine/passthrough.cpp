#include "engine/passthrough.h"

#include <juce_audio_basics/juce_audio_basics.h>

namespace
{
	constexpr int SILENCE = -1;
	constexpr int LEFT = 0;
	constexpr int RIGHT = 1;

	int existing_or_silence(int channel, int input_count)
	{
		return channel < input_count ? channel : SILENCE;
	}
}

void Passthrough::add_to(const float* const* inputs, int input_count, float* const* outputs, int output_count, int sample_count) const
{
	for (int output = 0; output < output_count; ++output) {
		const auto source = source_for(output, input_count);

		if (source != SILENCE)
			juce::FloatVectorOperations::add(outputs[output], inputs[source], sample_count);
	}
}

int Passthrough::source_for(int output_channel, int input_count) const
{
	if (!monitoring.load(std::memory_order_relaxed))
		return SILENCE;

	switch (routing.load(std::memory_order_relaxed)) {
		case InputRouting::left_to_both:
			return existing_or_silence(LEFT, input_count);
		case InputRouting::right_to_both:
			return existing_or_silence(RIGHT, input_count);
		case InputRouting::stereo:
			return existing_or_silence(output_channel, input_count);
	}

	return SILENCE;
}
