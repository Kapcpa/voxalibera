#include "engine/plugins/builtin/drive_pedal.h"

#include "engine/dsp/clipping.h"

namespace
{
	constexpr float DC_BLOCKER_HERTZ = 8.0f;
	constexpr float LEVEL_MINIMUM = -24.0f;
	constexpr float LEVEL_MAXIMUM = 12.0f;
}

DrivePedal::DrivePedal(int oversampling_power) : oversampler(oversampling_power)
{
}

BuiltinParameter& DrivePedal::add_level(const juce::String& name)
{
	return add_parameter({.key = name.toLowerCase(), .name = name, .minimum = LEVEL_MINIMUM, .maximum = LEVEL_MAXIMUM, .default_value = 0.0f, .unit = "dB"});
}

float DrivePedal::output_gain(float level_db, float trim_db) const
{
	return juce::Decibels::decibelsToGain(level_db + trim_db) / VOLTS_AT_FULL_SCALE;
}

void DrivePedal::prepare(const PluginProcessContext& context)
{
	oversampler.prepare(context.sample_rate, context.maximum_block_size);
	latency = oversampler.latency();

	for (auto& blocker : dc_blockers) {
		blocker.set_cutoff(DC_BLOCKER_HERTZ, context.sample_rate);
		blocker.reset();
	}

	prepare_circuit(oversampler.rate());
}

void DrivePedal::process(juce::AudioBuffer<float>& buffer, int sample_count, const PluginPlayhead&)
{
	buffer.applyGain(0, sample_count, VOLTS_AT_FULL_SCALE);
	oversampler.process(buffer, sample_count, [this](float* const* channels, int oversampled_count) { process_circuit(channels, oversampled_count); });

	for (int channel = 0; channel < CHANNELS; ++channel) {
		auto* samples = buffer.getWritePointer(channel);

		for (int index = 0; index < sample_count; ++index)
			samples[index] = dc_blockers[(size_t) channel].highpass(samples[index]);
	}
}
