#include "engine/level_meter.h"

#include <juce_audio_basics/juce_audio_basics.h>

void LevelMeter::measure(const float* const* channels, int channel_count, int sample_count)
{
	for (int channel = 0; channel < CHANNELS && channel < channel_count; ++channel) {
		const auto range = juce::FloatVectorOperations::findMinAndMax(channels[channel], sample_count);
		const auto peak = std::max(-range.getStart(), range.getEnd());
		auto& stored_peak = peaks[(size_t) channel];
		auto previous = stored_peak.load(std::memory_order_relaxed);

		while (peak > previous && !stored_peak.compare_exchange_weak(previous, peak, std::memory_order_relaxed)) {
		}
	}
}

float LevelMeter::take_peak(int channel)
{
	return peaks[(size_t) channel].exchange(0.0f, std::memory_order_relaxed);
}
