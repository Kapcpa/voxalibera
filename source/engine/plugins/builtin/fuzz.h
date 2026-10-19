#pragma once

#include "engine/dsp/clipping.h"
#include "engine/dsp/envelope_follower.h"
#include "engine/plugins/builtin/drive_pedal.h"

class Fuzz : public DrivePedal
{
public:
	Fuzz();

private:
	struct Stage
	{
		OnePole coupling_highpass;
		OnePole feedback_lowpass;
	};

	struct Channel
	{
		OnePole input_highpass;
		EnvelopeFollower envelope;
		std::array<Stage, 2> stages;
		OnePole tone_lowpass;
		OnePole tone_highpass;
		OnePole output_lowpass;
	};

	void prepare_circuit(double oversampled_rate) override;
	void process_circuit(float* const* channels, int sample_count) override;

	BuiltinParameter& sustain;
	BuiltinParameter& tone;
	BuiltinParameter& bias;
	BuiltinParameter& volume;
	juce::SmoothedValue<float> sustain_gain;
	juce::SmoothedValue<float> tone_mix;
	juce::SmoothedValue<float> bias_amount;
	juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> volume_gain;
	std::array<Channel, CHANNELS> circuit;
};
