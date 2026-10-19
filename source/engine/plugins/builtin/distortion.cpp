#include "engine/plugins/builtin/distortion.h"

#include <algorithm>
#include <cmath>

namespace
{
	constexpr float KNOB_MAXIMUM = 10.0f;
	constexpr float DISTORTION_POT_OHMS = 100.0e3f;
	constexpr float LOW_LEG_OHMS = 560.0f;
	constexpr double LOW_LEG_FARADS = 4.7e-6;
	constexpr float HIGH_LEG_OHMS = 47.0f;
	constexpr double HIGH_LEG_FARADS = 2.2e-6;
	constexpr float GAIN_BANDWIDTH_HERTZ = 6.0e6f;
	constexpr float NARROWEST_BANDWIDTH_HERTZ = 2000.0f;
	constexpr float WIDEST_BANDWIDTH_HERTZ = 20000.0f;
	constexpr double SLEW_VOLTS_PER_SECOND = 0.3e6;
	constexpr float RAIL_VOLTS = 4.5f;
	constexpr double FILTER_SERIES_OHMS = 1.5e3;
	constexpr double FILTER_POT_OHMS = 100.0e3;
	constexpr double FILTER_FARADS = 3.3e-9;
	constexpr float LOOSEST_TIGHT_HERTZ = 20.0f;
	constexpr float TIGHTEST_HERTZ = 400.0f;
	constexpr float TRIM_DB = -17.2f;

	float cutoff_of(double ohms, double farads)
	{
		return (float) (1.0 / (juce::MathConstants<double>::twoPi * ohms * farads));
	}
}

Distortion::Distortion() : DrivePedal(3), distortion(add_parameter({.key = "distortion", .name = "Distortion", .maximum = KNOB_MAXIMUM, .default_value = 5.0f})), filter(add_parameter({.key = "filter", .name = "Filter", .maximum = KNOB_MAXIMUM, .default_value = 3.0f})), tight(add_parameter({.key = "tight", .name = "Tight", .maximum = KNOB_MAXIMUM, .default_value = 2.0f})), volume(add_level("Volume")), diodes(add_parameter({.key = "diodes", .name = "Diodes", .choices = DIODE_CHOICES}))
{
}

void Distortion::prepare_circuit(double oversampled_rate)
{
	rate = oversampled_rate;
	slew_step = (float) (SLEW_VOLTS_PER_SECOND / rate);

	for (auto* smoothed : {&distortion_position, &filter_position, &tight_position})
		smoothed->reset(rate, SMOOTHING_SECONDS);

	volume_gain.reset(rate, SMOOTHING_SECONDS);
	distortion_position.setCurrentAndTargetValue(distortion.value() / KNOB_MAXIMUM);
	filter_position.setCurrentAndTargetValue(filter.value() / KNOB_MAXIMUM);
	tight_position.setCurrentAndTargetValue(tight.value() / KNOB_MAXIMUM);
	volume_gain.setCurrentAndTargetValue(output_gain(volume.value(), TRIM_DB));

	for (auto& channel : circuit) {
		channel = {};
		channel.low_leg.set_cutoff(cutoff_of(LOW_LEG_OHMS, LOW_LEG_FARADS), rate);
		channel.high_leg.set_cutoff(cutoff_of(HIGH_LEG_OHMS, HIGH_LEG_FARADS), rate);
	}

	set_controls(distortion_position.getCurrentValue(), filter_position.getCurrentValue(), tight_position.getCurrentValue());
}

void Distortion::set_controls(float distortion_position_now, float filter_position_now, float tight_position_now)
{
	distortion_ohms = DISTORTION_POT_OHMS * audio_taper(distortion_position_now);

	const auto treble_gain = 1.0f + distortion_ohms / LOW_LEG_OHMS + distortion_ohms / HIGH_LEG_OHMS;
	const auto bandwidth = std::clamp(GAIN_BANDWIDTH_HERTZ / treble_gain, NARROWEST_BANDWIDTH_HERTZ, WIDEST_BANDWIDTH_HERTZ);
	const auto filter_cutoff = cutoff_of(FILTER_SERIES_OHMS + FILTER_POT_OHMS * audio_taper(filter_position_now), FILTER_FARADS);
	const auto tight_cutoff = LOOSEST_TIGHT_HERTZ * std::pow(TIGHTEST_HERTZ / LOOSEST_TIGHT_HERTZ, tight_position_now);

	for (auto& channel : circuit) {
		channel.bandwidth_lowpass.set_cutoff(bandwidth, rate);
		channel.filter_lowpass.set_cutoff(filter_cutoff, rate);

		for (auto& pole : channel.tight_highpass)
			pole.set_cutoff(tight_cutoff, rate);
	}
}

void Distortion::process_circuit(float* const* channels, int sample_count)
{
	distortion_position.setTargetValue(distortion.value() / KNOB_MAXIMUM);
	filter_position.setTargetValue(filter.value() / KNOB_MAXIMUM);
	tight_position.setTargetValue(tight.value() / KNOB_MAXIMUM);
	volume_gain.setTargetValue(output_gain(volume.value(), TRIM_DB));
	const auto clipping = diodes_of(diode_type_at(diodes.value()), DiodePlacement::to_ground);

	for (int index = 0; index < sample_count; ++index) {
		if (distortion_position.isSmoothing() || filter_position.isSmoothing() || tight_position.isSmoothing())
			set_controls(distortion_position.getNextValue(), filter_position.getNextValue(), tight_position.getNextValue());

		const auto output = volume_gain.getNextValue();

		for (int channel = 0; channel < CHANNELS; ++channel) {
			auto& stage = circuit[(size_t) channel];
			auto& sample = channels[channel][index];

			auto input = sample;

			for (auto& pole : stage.tight_highpass)
				input = pole.highpass(input);

			const auto leg_current = stage.low_leg.highpass(input) / LOW_LEG_OHMS + stage.high_leg.highpass(input) / HIGH_LEG_OHMS;
			const auto amplified = stage.bandwidth_lowpass.lowpass(input + distortion_ohms * leg_current);
			stage.slewed += std::clamp(amplified - stage.slewed, -slew_step, slew_step);

			const auto railed = rail_clip(stage.slewed, RAIL_VOLTS);
			sample = stage.filter_lowpass.lowpass(diode_clip(railed, clipping)) * output;
		}
	}
}
