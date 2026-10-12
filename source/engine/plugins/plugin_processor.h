#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

struct PluginProcessContext
{
	double sample_rate;
	int buffer_size;

	PluginProcessContext(double rate, int buffer_size) : sample_rate(rate), buffer_size(buffer_size) {}
};

class PluginProcessor
{
public:
	virtual ~PluginProcessor() = default;

	virtual void prepare(const PluginProcessContext& context) = 0;
	virtual void reset() = 0;

	virtual void process(juce::AudioBuffer<float>& buffer, int num_samples) = 0;

	virtual int latency_samples() const noexcept { return 0; }
	virtual int tail_samples() const noexcept { return 0; }
};