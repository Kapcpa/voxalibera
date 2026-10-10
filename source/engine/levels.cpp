#include "engine/levels.h"

#include <juce_audio_basics/juce_audio_basics.h>

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
