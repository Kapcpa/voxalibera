#include "engine/plugins/builtin/noise_gate.h"

#include <cmath>

namespace
{
	constexpr float DETECTOR_ATTACK_SECONDS = 0.0005f;
	constexpr float DETECTOR_RELEASE_SECONDS = 0.02f;
	constexpr float OPENING_SECONDS = 0.001f;
	constexpr float HYSTERESIS_DB = 6.0f;
	constexpr float MILLISECONDS = 1000.0f;
}

NoiseGate::NoiseGate() : threshold(add_parameter({.key = "threshold", .name = "Threshold", .minimum = -90.0f, .maximum = 0.0f, .default_value = -60.0f, .unit = "dB"})), hold(add_parameter({.key = "hold", .name = "Hold", .maximum = 500.0f, .default_value = 30.0f, .unit = "ms", .decimals = 0, .centre = 80.0f})), release(add_parameter({.key = "release", .name = "Release", .minimum = 5.0f, .maximum = 1000.0f, .default_value = 120.0f, .unit = "ms", .decimals = 0, .centre = 150.0f})), range(add_parameter({.key = "range", .name = "Range", .minimum = -90.0f, .maximum = 0.0f, .default_value = -90.0f, .unit = "dB"}))
{
}

float NoiseGate::smoothing_for(float seconds) const
{
	return (float) std::exp(-1.0 / (seconds * sample_rate));
}

void NoiseGate::prepare(const PluginProcessContext& context)
{
	sample_rate = context.sample_rate;
	detector.set_times(DETECTOR_ATTACK_SECONDS, DETECTOR_RELEASE_SECONDS, sample_rate);
	detector.reset();
	gain = juce::Decibels::decibelsToGain(range.value());
	hold_left = 0;
	open = false;
}

void NoiseGate::process(juce::AudioBuffer<float>& buffer, int sample_count, const PluginPlayhead&)
{
	const auto opens_at = juce::Decibels::decibelsToGain(threshold.value());
	const auto closes_at = juce::Decibels::decibelsToGain(threshold.value() - HYSTERESIS_DB);
	const auto closed_gain = juce::Decibels::decibelsToGain(range.value(), -100.0f);
	const auto hold_samples = (int) (hold.value() / MILLISECONDS * sample_rate);
	const auto opening = smoothing_for(OPENING_SECONDS);
	const auto closing = smoothing_for(release.value() / MILLISECONDS);
	auto* const* channels = buffer.getArrayOfWritePointers();
	const auto channel_count = buffer.getNumChannels();

	for (int index = 0; index < sample_count; ++index) {
		float loudest = 0.0f;

		for (int channel = 0; channel < channel_count; ++channel)
			loudest = std::max(loudest, std::abs(channels[channel][index]));

		const auto level = detector.process(loudest);

		if (level >= opens_at) {
			open = true;
			hold_left = hold_samples;
		} else if (level < closes_at && hold_left > 0) {
			--hold_left;
		} else if (level < closes_at) {
			open = false;
		}

		const auto target = open ? 1.0f : closed_gain;
		const auto coefficient = target > gain ? opening : closing;
		gain = target + coefficient * (gain - target);

		for (int channel = 0; channel < channel_count; ++channel)
			channels[channel][index] *= gain;
	}
}
