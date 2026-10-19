#pragma once

#include "engine/dsp/clipping.h"
#include "engine/plugins/builtin/drive_pedal.h"

class Distortion : public DrivePedal
{
public:
	Distortion();

private:
	struct Channel
	{
		std::array<OnePole, 2> tight_highpass;
		OnePole low_leg;
		OnePole high_leg;
		OnePole bandwidth_lowpass;
		OnePole filter_lowpass;
		float slewed = 0.0f;
	};

	void prepare_circuit(double oversampled_rate) override;
	void process_circuit(float* const* channels, int sample_count) override;
	void set_controls(float distortion_position, float filter_position, float tight_position);

	BuiltinParameter& distortion;
	BuiltinParameter& filter;
	BuiltinParameter& tight;
	BuiltinParameter& volume;
	BuiltinParameter& diodes;
	juce::SmoothedValue<float> distortion_position;
	juce::SmoothedValue<float> filter_position;
	juce::SmoothedValue<float> tight_position;
	juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> volume_gain;
	std::array<Channel, CHANNELS> circuit;
	double rate = 0.0;
	float distortion_ohms = 0.0f;
	float slew_step = 0.0f;
};
