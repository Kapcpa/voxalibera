#pragma once

#include <juce_dsp/juce_dsp.h>

#include <memory>

class Oversampler
{
public:
	static constexpr int CHANNELS = 2;

	explicit Oversampler(int factor_power);

	void prepare(double sample_rate, int maximum_block_size);
	double rate() const { return oversampled_rate; }
	int latency() const;

	template <typename Process>
	void process(juce::AudioBuffer<float>& buffer, int sample_count, Process&& process_oversampled)
	{
		juce::dsp::AudioBlock<float> block(buffer.getArrayOfWritePointers(), (size_t) CHANNELS, (size_t) sample_count);
		auto oversampled = oversampling.processSamplesUp(block);
		float* channels[CHANNELS]{oversampled.getChannelPointer(0), oversampled.getChannelPointer(1)};
		process_oversampled(channels, (int) oversampled.getNumSamples());
		oversampling.processSamplesDown(block);
	}

private:
	juce::dsp::Oversampling<float> oversampling;
	double oversampled_rate = 0.0;
};
