#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <memory>

struct ImpulseResponse
{
	juce::AudioBuffer<float> samples;
	double sample_rate = 0.0;

	bool is_empty() const { return samples.getNumSamples() == 0; }
	float seconds() const;
};

std::unique_ptr<ImpulseResponse> read_impulse_response(const juce::File& file);
std::unique_ptr<ImpulseResponse> generic_cabinet_response();
std::unique_ptr<ImpulseResponse> no_impulse_response();
