#include "engine/plugins/builtin/blues_drive.h"

namespace
{
	constexpr float KNOB_MAXIMUM = 10.0f;
	constexpr float INPUT_HIGHPASS_HERTZ = 40.0f;
	constexpr float FIRST_STAGE_EXTRA_GAIN = 9.0f;
	constexpr float SECOND_STAGE_EXTRA_GAIN = 20.0f;
	constexpr float RAIL_VOLTS = 4.5f;
	constexpr float FEEDBACK_HIGHPASS_HERTZ = 300.0f;
	constexpr float STAGE_BANDWIDTH_HERTZ = 120000.0f;
	constexpr int DIODES_IN_SERIES = 2;
	constexpr float BIAS_DEPTH = 0.25f;
	constexpr float BIAS_ATTACK_SECONDS = 0.003f;
	constexpr float BIAS_RELEASE_SECONDS = 0.06f;
	constexpr float TILT_HERTZ = 900.0f;
	constexpr float TILT_RANGE_DB = 8.0f;
	constexpr float OUTPUT_LOWPASS_HERTZ = 8000.0f;
	constexpr float TRIM_DB = -8.7f;
}

BluesDrive::BluesDrive() : DrivePedal(2), gain(add_parameter({.key = "gain", .name = "Gain", .maximum = KNOB_MAXIMUM, .default_value = 5.0f})), tone(add_parameter({.key = "tone", .name = "Tone", .maximum = KNOB_MAXIMUM, .default_value = 5.0f})), volume(add_level("Volume")), diodes(add_parameter({.key = "diodes", .name = "Diodes", .choices = DIODE_CHOICES}))
{
}

void BluesDrive::prepare_circuit(double oversampled_rate)
{
	rate = oversampled_rate;

	for (auto* smoothed : {&gain_position, &tone_position})
		smoothed->reset(rate, SMOOTHING_SECONDS);

	volume_gain.reset(rate, SMOOTHING_SECONDS);
	gain_position.setCurrentAndTargetValue(gain.value() / KNOB_MAXIMUM);
	tone_position.setCurrentAndTargetValue(tone.value() / KNOB_MAXIMUM);
	volume_gain.setCurrentAndTargetValue(output_gain(volume.value(), TRIM_DB));

	for (auto& channel : circuit) {
		channel = {};
		channel.input_highpass.set_cutoff(INPUT_HIGHPASS_HERTZ, rate);
		channel.feedback_highpass.set_cutoff(FEEDBACK_HIGHPASS_HERTZ, rate);
		channel.tilt_lowpass.set_cutoff(TILT_HERTZ, rate);
		channel.output_lowpass.set_cutoff(OUTPUT_LOWPASS_HERTZ, rate);
		channel.envelope.set_times(BIAS_ATTACK_SECONDS, BIAS_RELEASE_SECONDS, rate);
	}

	set_controls(gain_position.getCurrentValue(), tone_position.getCurrentValue());
}

void BluesDrive::set_controls(float gain_position_now, float tone_position_now)
{
	const auto taper = audio_taper(gain_position_now);
	first_stage_gain = 1.0f + FIRST_STAGE_EXTRA_GAIN * taper;
	second_stage_gain = SECOND_STAGE_EXTRA_GAIN * taper;

	const auto tilt_db = (2.0f * tone_position_now - 1.0f) * TILT_RANGE_DB;
	bass_gain = juce::Decibels::decibelsToGain(-0.5f * tilt_db);
	treble_gain = juce::Decibels::decibelsToGain(0.5f * tilt_db);

	for (auto& channel : circuit)
		channel.feedback_lowpass.set_cutoff(STAGE_BANDWIDTH_HERTZ / (1.0f + second_stage_gain), rate);
}

void BluesDrive::process_circuit(float* const* channels, int sample_count)
{
	gain_position.setTargetValue(gain.value() / KNOB_MAXIMUM);
	tone_position.setTargetValue(tone.value() / KNOB_MAXIMUM);
	volume_gain.setTargetValue(output_gain(volume.value(), TRIM_DB));
	const auto clipping = diodes_of(diode_type_at(diodes.value()), DiodePlacement::feedback, DIODES_IN_SERIES);

	for (int index = 0; index < sample_count; ++index) {
		if (gain_position.isSmoothing() || tone_position.isSmoothing())
			set_controls(gain_position.getNextValue(), tone_position.getNextValue());

		const auto output = volume_gain.getNextValue();

		for (int channel = 0; channel < CHANNELS; ++channel) {
			auto& stage = circuit[(size_t) channel];
			auto& sample = channels[channel][index];

			const auto first = rail_clip(first_stage_gain * stage.input_highpass.highpass(sample), RAIL_VOLTS);
			const auto bias = BIAS_DEPTH * stage.envelope.process(first);
			const auto feedback = stage.feedback_lowpass.lowpass(second_stage_gain * stage.feedback_highpass.highpass(first));
			const auto second = first + diode_clip(feedback + bias, clipping) - diode_clip(bias, clipping);
			const auto bass = stage.tilt_lowpass.lowpass(second);
			const auto tilted = bass_gain * bass + treble_gain * (second - bass);
			sample = stage.output_lowpass.lowpass(tilted) * output;
		}
	}
}
