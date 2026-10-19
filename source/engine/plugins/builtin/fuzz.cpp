#include "engine/plugins/builtin/fuzz.h"

namespace
{
	constexpr float KNOB_MAXIMUM = 10.0f;
	constexpr float INPUT_HIGHPASS_HERTZ = 50.0f;
	constexpr float SUSTAIN_EXTRA_GAIN = 60.0f;
	constexpr float STAGE_GAIN = 30.0f;
	constexpr float COUPLING_HIGHPASS_HERTZ = 100.0f;
	constexpr float STAGE_LOWPASS_HERTZ = 4000.0f;
	constexpr float DEAD_ZONE_VOLTS = 0.4f;
	constexpr float BIAS_SHIFT = 0.5f;
	constexpr float ENVELOPE_ATTACK_SECONDS = 0.002f;
	constexpr float ENVELOPE_RELEASE_SECONDS = 0.1f;
	constexpr float TONE_LOWPASS_HERTZ = 408.0f;
	constexpr float TONE_HIGHPASS_HERTZ = 1200.0f;
	constexpr float OUTPUT_LOWPASS_HERTZ = 7000.0f;
	constexpr float TRIM_DB = -15.3f;

	float sustain_gain_at(float position)
	{
		return 1.0f + SUSTAIN_EXTRA_GAIN * audio_taper(position / KNOB_MAXIMUM);
	}
}

Fuzz::Fuzz() : DrivePedal(3), sustain(add_parameter({.key = "sustain", .name = "Sustain", .maximum = KNOB_MAXIMUM, .default_value = 6.0f})), tone(add_parameter({.key = "tone", .name = "Tone", .maximum = KNOB_MAXIMUM, .default_value = 5.0f})), bias(add_parameter({.key = "bias", .name = "Bias", .maximum = KNOB_MAXIMUM, .default_value = 0.0f})), volume(add_level("Volume"))
{
}

void Fuzz::prepare_circuit(double rate)
{
	for (auto* smoothed : {&sustain_gain, &tone_mix, &bias_amount})
		smoothed->reset(rate, SMOOTHING_SECONDS);

	volume_gain.reset(rate, SMOOTHING_SECONDS);
	sustain_gain.setCurrentAndTargetValue(sustain_gain_at(sustain.value()));
	tone_mix.setCurrentAndTargetValue(tone.value() / KNOB_MAXIMUM);
	bias_amount.setCurrentAndTargetValue(bias.value() / KNOB_MAXIMUM);
	volume_gain.setCurrentAndTargetValue(output_gain(volume.value(), TRIM_DB));

	for (auto& channel : circuit) {
		channel = {};
		channel.input_highpass.set_cutoff(INPUT_HIGHPASS_HERTZ, rate);
		channel.envelope.set_times(ENVELOPE_ATTACK_SECONDS, ENVELOPE_RELEASE_SECONDS, rate);
		channel.tone_lowpass.set_cutoff(TONE_LOWPASS_HERTZ, rate);
		channel.tone_highpass.set_cutoff(TONE_HIGHPASS_HERTZ, rate);
		channel.output_lowpass.set_cutoff(OUTPUT_LOWPASS_HERTZ, rate);

		for (auto& stage : channel.stages) {
			stage.coupling_highpass.set_cutoff(COUPLING_HIGHPASS_HERTZ, rate);
			stage.feedback_lowpass.set_cutoff(STAGE_LOWPASS_HERTZ, rate);
		}
	}
}

void Fuzz::process_circuit(float* const* channels, int sample_count)
{
	sustain_gain.setTargetValue(sustain_gain_at(sustain.value()));
	tone_mix.setTargetValue(tone.value() / KNOB_MAXIMUM);
	bias_amount.setTargetValue(bias.value() / KNOB_MAXIMUM);
	volume_gain.setTargetValue(output_gain(volume.value(), TRIM_DB));
	const auto clipping = diodes_of(DiodeType::silicon, DiodePlacement::feedback);

	for (int index = 0; index < sample_count; ++index) {
		const auto drive = sustain_gain.getNextValue();
		const auto mix = tone_mix.getNextValue();
		const auto starve = bias_amount.getNextValue();
		const auto output = volume_gain.getNextValue();

		for (int channel = 0; channel < CHANNELS; ++channel) {
			auto& path = circuit[(size_t) channel];
			auto& sample = channels[channel][index];

			const auto driven = drive * path.input_highpass.highpass(sample);
			const auto shift = starve * BIAS_SHIFT * path.envelope.process(driven);
			const auto starved = soft_dead_zone(driven, starve * DEAD_ZONE_VOLTS);

			auto& first = path.stages[0];
			const auto first_input = first.feedback_lowpass.lowpass(STAGE_GAIN * first.coupling_highpass.highpass(starved));
			const auto first_output = diode_clip(first_input + shift, clipping) - diode_clip(shift, clipping);

			auto& second = path.stages[1];
			const auto second_input = second.feedback_lowpass.lowpass(STAGE_GAIN * second.coupling_highpass.highpass(first_output));
			const auto fuzzed = diode_clip(second_input, clipping);

			const auto scooped = (1.0f - mix) * path.tone_lowpass.lowpass(fuzzed) + mix * path.tone_highpass.highpass(fuzzed);
			sample = path.output_lowpass.lowpass(scooped) * output;
		}
	}
}
