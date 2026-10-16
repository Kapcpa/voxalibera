#include "engine/plugins/builtin/test_drive.h"

#include <cmath>

namespace
{
	constexpr double SMOOTHING_SECONDS = 0.02;
	constexpr float PERCENT = 100.0f;
}

TestDrive::TestDrive() : drive(add_parameter({.key = "drive", .name = "Drive", .minimum = 0.0f, .maximum = 36.0f, .default_value = 8.0f, .unit = "dB"})), mix(add_parameter({.key = "mix", .name = "Mix", .minimum = 0.0f, .maximum = 100.0f, .default_value = 100.0f, .unit = "%", .decimals = 0})), output(add_parameter({.key = "output", .name = "Output", .minimum = -24.0f, .maximum = 24.0f, .default_value = 0.0f, .unit = "dB"}))
{
}

void TestDrive::prepare(const PluginProcessContext& context)
{
	for (auto* smoothed : {&drive_gain, &wet, &output_gain})
		smoothed->reset(context.sample_rate, SMOOTHING_SECONDS);

	drive_gain.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(drive.value()));
	wet.setCurrentAndTargetValue(mix.value() / PERCENT);
	output_gain.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(output.value()));
}

void TestDrive::process(juce::AudioBuffer<float>& buffer, int sample_count, const PluginPlayhead&)
{
	drive_gain.setTargetValue(juce::Decibels::decibelsToGain(drive.value()));
	wet.setTargetValue(mix.value() / PERCENT);
	output_gain.setTargetValue(juce::Decibels::decibelsToGain(output.value()));

	auto* const* channels = buffer.getArrayOfWritePointers();
	const auto channel_count = buffer.getNumChannels();

	for (int index = 0; index < sample_count; ++index) {
		const auto gain = drive_gain.getNextValue();
		const auto wet_amount = wet.getNextValue();
		const auto level = output_gain.getNextValue();

		for (int channel = 0; channel < channel_count; ++channel) {
			auto& sample = channels[channel][index];
			sample = (sample + wet_amount * (std::tanh(sample * gain) - sample)) * level;
		}
	}
}
