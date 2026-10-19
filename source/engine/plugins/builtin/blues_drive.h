#pragma once

#include "engine/dsp/clipping.h"
#include "engine/dsp/envelope_follower.h"
#include "engine/plugins/builtin/drive_pedal.h"

class BluesDrive : public DrivePedal
{
public:
	BluesDrive();

private:
	struct Channel
	{
		OnePole input_highpass;
		OnePole feedback_highpass;
		OnePole feedback_lowpass;
		OnePole tilt_lowpass;
		OnePole output_lowpass;
		EnvelopeFollower envelope;
	};

	void prepare_circuit(double oversampled_rate) override;
	void process_circuit(float* const* channels, int sample_count) override;
	void set_controls(float gain_position, float tone_position);

	BuiltinParameter& gain;
	BuiltinParameter& tone;
	BuiltinParameter& volume;
	BuiltinParameter& diodes;
	juce::SmoothedValue<float> gain_position;
	juce::SmoothedValue<float> tone_position;
	juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> volume_gain;
	std::array<Channel, CHANNELS> circuit;
	double rate = 0.0;
	float first_stage_gain = 1.0f;
	float second_stage_gain = 0.0f;
	float bass_gain = 1.0f;
	float treble_gain = 1.0f;
};
