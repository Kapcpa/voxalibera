#include "engine/plugins/builtin/overdrive.h"

#include <cmath>

namespace
{
	constexpr float KNOB_MAXIMUM = 10.0f;
	constexpr double DRIVE_POT_OHMS = 500.0e3;
	constexpr double DRIVE_SERIES_OHMS = 51.0e3;
	constexpr double GROUND_LEG_OHMS = 4.7e3;
	constexpr double GROUND_LEG_FARADS = 47.0e-9;
	constexpr double FEEDBACK_FARADS = 51.0e-12;
	constexpr float TONE_LOWPASS_HERTZ = 723.0f;
	constexpr float DARKEST_TREBLE_DB = -26.0f;
	constexpr float BRIGHTEST_TREBLE_DB = 4.0f;
	constexpr float OUTPUT_LOWPASS_HERTZ = 7500.0f;
	constexpr float TRIM_DB = -9.0f;

	float cutoff_of(double ohms, double farads)
	{
		return (float) (1.0 / (juce::MathConstants<double>::twoPi * ohms * farads));
	}
}

Overdrive::Overdrive() : DrivePedal(2), drive(add_parameter({.key = "drive", .name = "Drive", .maximum = KNOB_MAXIMUM, .default_value = 5.0f})), tone(add_parameter({.key = "tone", .name = "Tone", .maximum = KNOB_MAXIMUM, .default_value = 5.0f})), level(add_level("Level")), diodes(add_parameter({.key = "diodes", .name = "Diodes", .choices = DIODE_CHOICES}))
{
}

void Overdrive::prepare_circuit(double oversampled_rate)
{
	rate = oversampled_rate;

	for (auto* smoothed : {&drive_position, &tone_position})
		smoothed->reset(rate, SMOOTHING_SECONDS);

	level_gain.reset(rate, SMOOTHING_SECONDS);
	drive_position.setCurrentAndTargetValue(drive.value() / KNOB_MAXIMUM);
	tone_position.setCurrentAndTargetValue(tone.value() / KNOB_MAXIMUM);
	level_gain.setCurrentAndTargetValue(output_gain(level.value(), TRIM_DB));

	for (auto& channel : circuit) {
		channel = {};
		channel.feedback_highpass.set_cutoff(cutoff_of(GROUND_LEG_OHMS, GROUND_LEG_FARADS), rate);
		channel.tone_lowpass.set_cutoff(TONE_LOWPASS_HERTZ, rate);
		channel.output_lowpass.set_cutoff(OUTPUT_LOWPASS_HERTZ, rate);
	}

	set_controls(drive_position.getCurrentValue(), tone_position.getCurrentValue());
}

void Overdrive::set_controls(float drive_position_now, float tone_position_now)
{
	const auto feedback_ohms = DRIVE_SERIES_OHMS + DRIVE_POT_OHMS * audio_taper(drive_position_now);
	feedback_gain = (float) (feedback_ohms / GROUND_LEG_OHMS);
	treble_gain = juce::Decibels::decibelsToGain(juce::jmap(tone_position_now, DARKEST_TREBLE_DB, BRIGHTEST_TREBLE_DB));

	for (auto& channel : circuit)
		channel.feedback_lowpass.set_cutoff(cutoff_of(feedback_ohms, FEEDBACK_FARADS), rate);
}

void Overdrive::process_circuit(float* const* channels, int sample_count)
{
	drive_position.setTargetValue(drive.value() / KNOB_MAXIMUM);
	tone_position.setTargetValue(tone.value() / KNOB_MAXIMUM);
	level_gain.setTargetValue(output_gain(level.value(), TRIM_DB));
	const auto clipping = diodes_of(diode_type_at(diodes.value()), DiodePlacement::feedback);

	for (int index = 0; index < sample_count; ++index) {
		if (drive_position.isSmoothing() || tone_position.isSmoothing())
			set_controls(drive_position.getNextValue(), tone_position.getNextValue());

		const auto gain = level_gain.getNextValue();

		for (int channel = 0; channel < CHANNELS; ++channel) {
			auto& stage = circuit[(size_t) channel];
			auto& sample = channels[channel][index];

			const auto mids = stage.feedback_highpass.highpass(sample);
			const auto feedback = stage.feedback_lowpass.lowpass(feedback_gain * mids);
			const auto clipped = sample + diode_clip(feedback, clipping);
			const auto dark = stage.tone_lowpass.lowpass(clipped);
			const auto toned = dark + treble_gain * (clipped - dark);
			sample = stage.output_lowpass.lowpass(toned) * gain;
		}
	}
}
