#include "engine/dsp/oversampler.h"

#include <cmath>

Oversampler::Oversampler(int factor_power) : oversampling((size_t) CHANNELS, (size_t) factor_power, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true)
{
}

void Oversampler::prepare(double sample_rate, int maximum_block_size)
{
	oversampling.initProcessing((size_t) maximum_block_size);
	oversampling.reset();
	oversampled_rate = sample_rate * (double) oversampling.getOversamplingFactor();
}

int Oversampler::latency() const
{
	return (int) std::lround(oversampling.getLatencyInSamples());
}
