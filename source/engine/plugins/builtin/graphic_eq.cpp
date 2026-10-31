#include "engine/plugins/builtin/graphic_eq.h"

GraphicEq::GraphicEq()
{
	for (size_t band = 0; band < BANDS; ++band)
		band_levels[band] = &add_parameter({.key = "band-" + juce::String(band + 1),
											.name = juce::String(BAND_FREQUENCIES[band]) + " Hz",
											.minimum = -15.0f,
											.maximum = 15.0f,
											.unit = "dB"});

	level_gain = &add_parameter({.key = "level-gain", .name = "Level Gain", .minimum = -24.0f, .maximum = 12.0f, .unit = "dB"});
}

void GraphicEq::set_band(size_t band, float gain_db)
{
	const auto coefficients = SvfCoefficients::make(FilterShape::bell, sample_rate, BAND_FREQUENCIES[band], octave_bandwidth_q(1.0f), gain_db);

	for (auto& filter : filter_state[band])
		filter.set(coefficients);
}

void GraphicEq::prepare(const PluginProcessContext& context)
{
	sample_rate = context.sample_rate;

	smoothed_level_gain.reset(sample_rate, SMOOTHING_SECONDS);
	smoothed_level_gain.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(level_gain->value()));

	for (size_t band = 0; band < BANDS; ++band) {
		smoothed_gains[band].reset(sample_rate, SMOOTHING_SECONDS);
		smoothed_gains[band].setCurrentAndTargetValue(band_levels[band]->value());

		for (auto& filter : filter_state[band])
			filter.reset();

		set_band(band, smoothed_gains[band].getCurrentValue());
	}
}

void GraphicEq::process(juce::AudioBuffer<float>& buffer, int sample_count, const PluginPlayhead&)
{
	smoothed_level_gain.setTargetValue(juce::Decibels::decibelsToGain(level_gain->value()));

	for (size_t band = 0; band < BANDS; ++band)
		smoothed_gains[band].setTargetValue(band_levels[band]->value());

	for (int index = 0; index < sample_count; ++index) {
		const auto level = smoothed_level_gain.getNextValue();

		for (size_t band = 0; band < BANDS; ++band)
			if (smoothed_gains[band].isSmoothing())
				set_band(band, smoothed_gains[band].getNextValue());

		for (size_t channel = 0; channel < CHANNELS; ++channel) {
			auto& sample = buffer.getWritePointer(channel)[index];

			for (size_t band = 0; band < BANDS; ++band)
				sample = filter_state[band][(size_t) channel].process(sample);

			sample *= level;
		}
	}
}
