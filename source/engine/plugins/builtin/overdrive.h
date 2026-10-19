#pragma once

#include "engine/dsp/clipping.h"
#include "engine/plugins/builtin/drive_pedal.h"

class Overdrive : public DrivePedal
{
public:
	Overdrive();

private:
	struct Channel
	{
		OnePole feedback_highpass;
		OnePole feedback_lowpass;
		OnePole tone_lowpass;
		OnePole output_lowpass;
	};

	void prepare_circuit(double oversampled_rate) override;
	void process_circuit(float* const* channels, int sample_count) override;
	void set_controls(float drive_position, float tone_position);

	BuiltinParameter& drive;
	BuiltinParameter& tone;
	BuiltinParameter& level;
	BuiltinParameter& diodes;
	juce::SmoothedValue<float> drive_position;
	juce::SmoothedValue<float> tone_position;
	juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> level_gain;
	std::array<Channel, CHANNELS> circuit;
	double rate = 0.0;
	float feedback_gain = 0.0f;
	float treble_gain = 1.0f;
};
